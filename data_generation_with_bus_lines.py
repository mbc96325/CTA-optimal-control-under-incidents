import pandas as pd
import networkx as nx

if __name__ == '__main__':
    id=0
    # load data
    stops = pd.read_csv('../Data/gtfs/stops.txt',sep = ',')
    schedule = pd.read_csv('../Data/gtfs/stop_times.txt', sep=',')
    calendar = pd.read_csv('../Data/gtfs/calendar.txt', sep=',')
    calendar_tuesday = calendar.loc[(calendar['tuesday'] == 1) &
                                    (calendar['end_date'] == 20191231)]

    schedule_bus = schedule.loc[(schedule['stop_id']<30000)]
    schedule_bus = schedule_bus.merge(stops[['stop_id','stop_name']], on = ['stop_id'])
    schedule_bus = schedule_bus.sort_values(['trip_id','stop_sequence'])

    trips = pd.read_csv('../Data/gtfs/trips.txt', sep=',')
    schedule_bus = schedule_bus.merge(trips[['route_id', 'service_id', 'trip_id', 'direction_id']], on=['trip_id'])
    schedule_bus = schedule_bus.merge(calendar_tuesday[['service_id']], on=['service_id'], how='inner')
    schedule_bus = schedule_bus.reset_index(drop=True)

    schedule_bus['departure_time'] = schedule_bus['departure_time'].apply(
        lambda x: int(x.split(':')[0]) * 3600 + int(x.split(':')[1]) * 60 + int(x.split(':')[2]))
    schedule_bus['arrival_time'] = schedule_bus['arrival_time'].apply(
        lambda x: int(x.split(':')[0]) * 3600 + int(x.split(':')[1]) * 60 + int(x.split(':')[2]))

    ####################
    #selected stations to control(and bus routes to take)

    selected_lines=['52', '74', '56', '73', '56', 'X49', '72', '56', '50']
    schedule_bus_selected=schedule_bus.loc[schedule_bus['route_id'].isin(selected_lines)].drop_duplicates()

    schedule_bus_selected['line_id']=schedule_bus_selected['route_id']
    schedule_bus_selected.loc[schedule_bus_selected['route_id'] == 'X49', 'line_id'] = '490'
    schedule_bus_selected['line_id']=schedule_bus_selected['line_id'].apply(lambda x:int(x))

    #selected trips

    bus_sequence = schedule_bus_selected[['service_id', 'stop_id', 'stop_sequence','line_id','stop_name',
                                             'route_id', 'direction_id']].drop_duplicates()
    bus_sequence = bus_sequence.reset_index(drop=True)

    #mark terminals

    bus_sequence['maxid']=bus_sequence.groupby(['line_id','direction_id'])['stop_sequence'].transform('max')
    bus_sequence.loc[
        (bus_sequence.stop_sequence == bus_sequence.maxid) & (bus_sequence.direction_id == 0), 'is_terminal0'] = True
    bus_sequence.loc[
        (bus_sequence.stop_sequence == bus_sequence.maxid) & (bus_sequence.direction_id == 1), 'is_terminal1'] = True
    bus_sequence['is_terminal0']=bus_sequence['is_terminal0'].fillna(False)
    bus_sequence['is_terminal1']=bus_sequence['is_terminal1'].fillna(False)

    #check terminals
    bus_terminal0 = bus_sequence.drop(columns=['service_id']).loc[(bus_sequence.is_terminal0 == True)]\
        .drop_duplicates().sort_values(['line_id', 'direction_id'])
    bus_terminal1 = bus_sequence.drop(columns=['service_id']).loc[(bus_sequence.is_terminal1 == True)]\
        .drop_duplicates().sort_values(['line_id', 'direction_id'])
    bus_terminals = bus_sequence.loc[(bus_sequence.is_terminal0 == True)
                                  | (bus_sequence.is_terminal1 == True)].drop(
        columns=['service_id', 'stop_sequence']).drop_duplicates().sort_values(['line_id'])

    # recoding bus stations to follow rail
    bus_stations = bus_sequence.drop(columns=['direction_id','service_id','stop_sequence','maxid','is_terminal0','is_terminal1'])\
        .merge(bus_terminals[['stop_id','line_id','is_terminal0','is_terminal1']],on=['stop_id','line_id'],how='left')
    bus_stations=bus_stations.drop_duplicates().sort_values(['line_id']).reset_index(drop=True)
    bus_stations['station_id']=bus_stations.index+252
    bus_stations=bus_stations.fillna(False)

    bus_stations.to_csv('check_bus_stations.csv')

    #note : terminal info in bus_sequence not true
    bus_sequence=bus_sequence.merge(bus_stations[['stop_id','line_id','station_id']],on=['stop_id','line_id'],how='left')
    schedule_bus_selected=schedule_bus_selected.merge(bus_stations[['stop_id','line_id','station_id']],on=['stop_id','line_id'],how='left')

    # import existing transfers
    rail_link=pd.read_csv('intermediate_data/rail_link.csv',sep = ',')


    ################################
    # add bus-rail transfers by searching map(50:none 52:green X49:none 56:none 72/73/74:brown/red/purple)
    #bus_rail_link=rail_link.copy()
    #52-green:444/596-62
    #72-red:822/842-40
    #73-brown:910/922-154/156
    #74-red/brown/purple:1024/1130-70/220/221/222/223
    bus_rail_link=pd.DataFrame(data={'from_station_id':[444,596,822,842,910,922,910,922,1024,1130,1024,1130,1024,1130,1024,1130,1024,1130],
                                     'to_station_id':[62,62,40,40,154,154,156,156,70,70,220,220,221,221,222,222,223,223]})
    bus_rail_link['time']=30

    ################################
    # mark bus transfer stations
    bus_transfers=bus_rail_link[['from_station_id']].drop_duplicates()
    bus_transfers=bus_transfers.rename(columns={'from_station_id':'station_id'})
    bus_transfers['is_transfer']=True

    bus_stations=bus_stations.merge(bus_transfers,on=['station_id'],how='left')
    bus_stations=bus_stations.fillna(False)

    bus_stations[['station_id','line_id','is_terminal0','is_terminal1','is_transfer']]\
        .to_csv('data_with_bus/stations.csv', index=False, header=False)

    # transfer time
    rail_transfer_link=pd.read_csv('intermediate_data/rail_transfer_link.csv')

    bus_not_transfer=bus_stations[['station_id']]
    bus_not_transfer=bus_not_transfer.rename(columns={'station_id':'from_station_id'})
    bus_not_transfer['to_station_id']=bus_not_transfer['from_station_id']
    bus_not_transfer['time']=-1

    all_transfer_link=pd.concat([rail_transfer_link,bus_rail_link,bus_not_transfer])
    all_transfer_link=pd.pivot(all_transfer_link,index='from_station_id',columns='to_station_id',values='time')

    all_transfer_link=all_transfer_link.fillna(-1)
    all_transfer_link.to_csv('data_with_bus/transferTime.csv', index=False, header=False)


    # sequential link
    schedule_bus_selected_shift = schedule_bus_selected.loc[:,
                          ['stop_id', 'trip_id', 'stop_sequence', 'departure_time',
                           'station_id']].shift(-1).reset_index()
    schedule_bus_selected['index']=schedule_bus_selected.index
    schedule_bus_selected_shift['stop_sequence_shift'] = schedule_bus_selected_shift['stop_sequence'] - 1
    bus_link = schedule_bus_selected.merge(schedule_bus_selected_shift, left_on=['index', 'trip_id', 'stop_sequence'],
                                    right_on=['index', 'trip_id', 'stop_sequence_shift'])

    bus_link['time'] = bus_link['departure_time_y'] - bus_link['departure_time_x']
    bus_link = bus_link.rename(columns={'station_id_x': 'from_station_id', 'station_id_y': 'to_station_id',})

    # direction for sequential links
    rail_direction=pd.read_csv('intermediate_data/directions.csv')
    rail_direction=rail_direction.rename(columns={'link_start':'from_station_id','link_end':'to_station_id'})

    bus_direction=bus_link[['from_station_id','to_station_id','direction_id']].drop_duplicates()

    all_directions=pd.concat([rail_direction,bus_direction])
    all_directions.to_csv('data_with_bus/directions.csv', index=False, header=False)

    # calc avg travel time
    bus_link = bus_link[['from_station_id','to_station_id','time']].drop_duplicates()
    bus_link = bus_link.groupby(['from_station_id','to_station_id']).mean().reset_index()


    # arrival time
    rail_arrivals=pd.read_csv('intermediate_data/arrivalTime.csv')
    rail_arrivals.columns=rail_arrivals.columns.astype(int)
    rail_num=len(rail_arrivals)

    schedule_bus_selected.loc[schedule_bus_selected['direction_id']==1,'trip_id']-=6270800000000

    bus_arrivals = pd.pivot(schedule_bus_selected, index='trip_id', columns='stop_sequence',
                            values='arrival_time').drop(columns=[1])
    bus_arrivals=bus_arrivals.reset_index(drop=True)
    bus_arrivals.index=bus_arrivals.index+rail_num

    all_arrivals = pd.concat([rail_arrivals,bus_arrivals])
    all_arrivals.to_csv('data_with_bus/arrivalTime.csv',index=False,header=False)

    # arrival stations
    rail_arrival_stations = pd.read_csv('intermediate_data/arrivalStationID.csv')
    rail_arrival_stations.columns = rail_arrival_stations.columns.astype(int)

    bus_arrival_stations = pd.pivot(schedule_bus_selected, index='trip_id', columns='stop_sequence',
                            values='station_id').drop(columns=[1])
    bus_arrival_stations = bus_arrival_stations.reset_index(drop=True)
    bus_arrival_stations.index = bus_arrival_stations.index + rail_num

    all_arrival_stations = pd.concat([rail_arrival_stations, bus_arrival_stations])
    all_arrival_stations.to_csv('data_with_bus/arrivalStationID.csv', index=False, header=False)

    # start train info
    start_rail=pd.read_csv('intermediate_data/startTrainInfo.csv')
    start_bus = schedule_bus_selected.loc[schedule_bus_selected['stop_sequence'] == 1].sort_values(['trip_id'])
    start_bus=start_bus.reset_index(drop=True)
    start_bus['trip_id']=start_bus.index+rail_num
    start_bus['capacity']=80

    all_start_stations=pd.concat([start_rail,start_bus[['trip_id','station_id','line_id','direction_id','capacity','departure_time']]])
    all_start_stations.to_csv('data_with_bus/startTrainInfo.csv', index=False, header=False)



    #############################################
    #network
    edges=pd.concat([bus_link,rail_link,bus_rail_link])
    G=nx.from_pandas_edgelist(edges,'from_station_id','to_station_id','time', create_using=nx.DiGraph())

    paths = nx.shortest_path(G, weight='time')
    df_paths = pd.DataFrame.from_dict(paths, orient='index')

    # policy
    next_stations=df_paths.stack().reset_index()
    next_stations=next_stations.rename(columns={'level_0':'from','level_1':'to',0:'next'})
    next_stations['next'] = next_stations['next'].apply(lambda x: x[1] if len(x) != 1 else -1)
    policy=next_stations.loc[next_stations.next!=-1]

    policy.to_csv('data_with_bus/policy.csv',index=False,header=False)

    # policy number
    next_stations['num'] = 1
    policy_num = pd.pivot(next_stations, index='from', columns='to', values='num')
    policy_num = policy_num.fillna(0)
    policy_num.to_csv('data_with_bus/policy_num.csv', index=False, header=False)


