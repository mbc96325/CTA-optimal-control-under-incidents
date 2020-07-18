import pandas as pd
import numpy as np
import networkx as nx


def add_parent_transfer(df):
    global transfer_link
    global id
    transfer_group=df.dropna().values.tolist()
    for i in range(len(transfer_group)) :
        for j in range(len(transfer_group)):
            if i!=j:
                transfer_link=pd.concat([transfer_link,pd.DataFrame(
                    {'from_station_id':transfer_group[i],'to_station_id':transfer_group[j],'time':30},index=[id])])
                id+=1


if __name__ == '__main__':
    id=0
    # load data
    stops = pd.read_csv('../Data/gtfs/stops.txt',sep = ',')
    schedule = pd.read_csv('../Data/gtfs/stop_times.txt' ,sep = ',')
    calendar = pd.read_csv('../Data/gtfs/calendar.txt',sep = ',')
    calendar_tuesday = calendar.loc[(calendar['tuesday'] == 1)&
                                       (calendar['end_date'] == 20191231)]

    schedule_rail = schedule.loc[(schedule['stop_id']>=30000) & (schedule['stop_id']<40000)]
    schedule_rail = schedule_rail.merge(stops[['stop_id','stop_name','parent_station']], on = ['stop_id'])
    schedule_rail = schedule_rail.sort_values(['trip_id','stop_sequence'])

    trips = pd.read_csv('../Data/gtfs/trips.txt', sep=',')
    schedule_rail = schedule_rail.merge(trips[['route_id', 'service_id', 'trip_id', 'direction_id']], on=['trip_id'])
    schedule_rail = schedule_rail.merge(calendar_tuesday[['service_id']], on=['service_id'], how='inner')
    schedule_rail = schedule_rail.reset_index(drop=True)

    schedule_rail['index'] = schedule_rail.index

    schedule_rail['departure_time'] = schedule_rail['departure_time'].apply(
            lambda x: int(x.split(':')[0]) * 3600 + int(x.split(':')[1]) * 60 + int(x.split(':')[2]))
    schedule_rail['arrival_time'] = schedule_rail['arrival_time'].apply(
            lambda x: int(x.split(':')[0]) * 3600 + int(x.split(':')[1]) * 60 + int(x.split(':')[2]))

    schedule_rail.loc[schedule_rail.route_id=='Blue','line_id']=1
    schedule_rail.loc[schedule_rail.route_id=='Brn','line_id']=3
    schedule_rail.loc[schedule_rail.route_id=='G','line_id']=4
    schedule_rail.loc[schedule_rail.route_id=='Org','line_id']=5
    schedule_rail.loc[schedule_rail.route_id=='P','line_id']=7
    schedule_rail.loc[schedule_rail.route_id=='Red','line_id']=8
    schedule_rail.loc[schedule_rail.route_id=='Pink','line_id']=6
    schedule_rail.loc[schedule_rail.route_id=='Y','line_id']=9

    #########################
    # should split short trips
    schedule_rail.loc[(schedule_rail['parent_station'] == schedule_rail['parent_station'].shift(1))
                      & (schedule_rail['stop_sequence'] != 1), 'split'] = True
    schedule_rail.loc[schedule_rail['stop_sequence'] == 1, 'split'] = False
    schedule_rail = schedule_rail.fillna(axis=0, method='ffill')

    schedule_rail.loc[schedule_rail['split'] == True, 'trip_id'] -= 59000000000
    schedule_rail['new_sequence'] = schedule_rail['stop_sequence'].groupby(schedule_rail['trip_id']).rank()

    schedule_rail.loc[(schedule_rail.line_id==3) | (schedule_rail.line_id==7), 'direction_id']=0

    #station
    station_sequence=schedule_rail.loc[:,['service_id','stop_id','stop_sequence','parent_station',
                                          'route_id','direction_id','line_id']].drop_duplicates()


    station_sequence=station_sequence.reset_index()
    stations=station_sequence.drop(columns=['direction_id']).drop_duplicates().sort_values(['line_id','stop_sequence'])


    is_terminal=station_sequence.copy().drop(columns=['service_id','index'])
    is_terminal['is_terminal0']=False
    is_terminal['is_terminal1']=False
    is_terminal['maxid']=station_sequence.groupby(['line_id','direction_id'])['stop_sequence'].transform('max')
    is_terminal.loc[(is_terminal.stop_sequence==is_terminal.maxid) &(is_terminal.direction_id==0),'is_terminal0']=True
    is_terminal.loc[(is_terminal.stop_sequence==is_terminal.maxid) &(is_terminal.direction_id==1),'is_terminal1']=True


    terminal0=is_terminal.loc[(is_terminal.is_terminal0==True)].drop_duplicates().sort_values(['line_id','direction_id'])
    terminal1=is_terminal.loc[(is_terminal.is_terminal1==True)].drop_duplicates().sort_values(['line_id','direction_id'])

    stations=stations.merge(terminal0[['line_id','parent_station','is_terminal0']],on=['line_id','parent_station'],how='left')
    stations=stations.merge(terminal1[['line_id','parent_station','is_terminal1']],on=['line_id','parent_station'],how='left')

    stations.loc[(stations.is_terminal0!=True)&(stations.is_terminal0!=False),'is_terminal0']=False
    stations.loc[(stations.is_terminal1!=True)&(stations.is_terminal1!=False),'is_terminal1']=False

    #check terminal
    terminal_check=stations.loc[(stations.is_terminal0==True)
                         |(stations.is_terminal1==True)].drop(columns=['service_id','index','stop_sequence'])\
        .drop_duplicates().sort_values(['line_id'])

    stations=stations.drop(columns=['service_id','index','stop_sequence']).drop_duplicates().reset_index()
    stations=stations.sort_values(['parent_station','stop_id'])



    transfers=pd.read_csv('../Data/gtfs/transfers.txt', sep = ',')
    transfers.loc[:,['transfer_type']]=True
    transfers=transfers.rename(columns={'transfer_type':'is_transfer'})

    is_transfer=pd.concat([transfers[['from_stop_id','is_transfer']],transfers[['to_stop_id','is_transfer']]]).drop_duplicates()
    is_transfer=is_transfer.rename(columns={'from_stop_id':'stop_id'})
    is_transfer.loc[is_transfer['stop_id'].isna(),'stop_id']=is_transfer.loc[is_transfer['stop_id'].isna(),'to_stop_id']
    is_transfer=is_transfer.drop(columns=['to_stop_id'])
    is_transfer=is_transfer.drop_duplicates()

    #check transfer
    #is_transfer2=pd.concat([transfers['from_stop_id'],transfers['to_stop_id']]).drop_duplicates()

    stations=stations.merge(is_transfer,on=['stop_id'],how='left')
    stations.loc[(stations.is_transfer!=True)&(stations.is_transfer!=False),'is_transfer']=False

    parent_stations=stations.loc[:,['parent_station','line_id','is_terminal0','is_terminal1','is_transfer']]\
        .drop_duplicates().reset_index(drop=True)
    parent_stations=parent_stations[~((parent_stations['parent_station']==41160) & (parent_stations['is_transfer']==False))]
    parent_stations['station_id']=parent_stations.index

    encode_part1=parent_stations.loc[(parent_stations.line_id==1)|(parent_stations.line_id==4)|
                                     (parent_stations.line_id==8)]
    encode_part1=encode_part1.sort_values(['parent_station','line_id']).reset_index(drop=True)
    encode_part1['station_id']=encode_part1.index

    encode_part2=stations.loc[(stations.line_id!=1)&(stations.line_id!=4)&
                                     (stations.line_id!=8)].reset_index(drop=True)
    encode_part2['station_id']=encode_part2.index+96
    encode_part2['is_dup']=encode_part2.duplicated(['parent_station','line_id'],keep=False)
    encode_part2.loc[encode_part2.is_dup==True,'is_transfer']=True

    encoded_stations=pd.concat([encode_part1.loc[:,['station_id','line_id','is_terminal0','is_terminal1', 'is_transfer']],
                                encode_part2.loc[:,['station_id','line_id','is_terminal0','is_terminal1', 'is_transfer']]],
                                ignore_index=True)
    # #encoded_stations.to_csv('data/stations.csv',index=False,header=False)

    #add info to stations(for use later
    stations=stations.merge(encode_part2[['stop_id','line_id','station_id']],
                                            on=['stop_id','line_id'],how='left',validate="m:1")
    stations=stations.merge(encode_part1[['parent_station','line_id','station_id']],
                                         on=['parent_station','line_id'],how='left',validate="m:1")
    stations=stations.fillna(axis=1,method='ffill')
    stations=stations.drop(columns=['station_id_x']).rename(columns={'station_id_y':'station_id'})


    #arrival_time
    arrivals=pd.pivot(schedule_rail,index='trip_id',columns='new_sequence',values='arrival_time')
    arrivals=arrivals.drop(columns=[1])

    arrivals.to_csv('data/arrivalTime.csv',index=False,header=False)

    #arrival_stations
    arrival_stations=schedule_rail.merge(encode_part2[['stop_id','line_id','station_id']],
                                            on=['stop_id','line_id'],how='left',validate="m:1")
    arrival_stations=arrival_stations.merge(encode_part1[['parent_station','line_id','station_id']],
                                         on=['parent_station','line_id'],how='left',validate="m:1")

    arrival_stations.loc[arrival_stations['station_id_x'].isna(),'station_id_x']\
        =arrival_stations.loc[arrival_stations['station_id_x'].isna(),'station_id_y']
    arrival_stations=arrival_stations.drop(columns=['station_id_y']).rename(columns={'station_id_x':'station_id'})
    arrival_station_id=pd.pivot(arrival_stations,index='trip_id',columns='new_sequence',values='station_id')
    arrival_station_id=arrival_station_id.drop(columns=[1])

    arrival_station_id.to_csv('data/arrivalStationID.csv',index=False,header=False)

    #start train info
    start_stations=schedule_rail.loc[schedule_rail.new_sequence==1].sort_values(['trip_id'])
    start_stations=start_stations.merge(encode_part2[['stop_id','line_id','station_id']],
                                            on=['stop_id','line_id'],how='left',validate="m:1")
    start_stations=start_stations.merge(encode_part1[['parent_station','line_id','station_id']],
                                         on=['parent_station','line_id'],how='left',validate="m:1")
    start_stations.loc[start_stations['station_id_x'].isna(),'station_id_x']\
        =start_stations.loc[start_stations['station_id_x'].isna(),'station_id_y']
    start_stations=start_stations.drop(columns=['station_id_y']).rename(columns={'station_id_x':'station_id'})
    #start_stations['capacity']=500
    start_stations=start_stations.sort_values('trip_id').reset_index(drop=True)
    start_stations['trip_id']=start_stations.index

    #add car length info
    schedule_car_length=pd.read_csv("schd_rail_trips_version_58.csv")

    schedule_car_length.loc[schedule_car_length.route == 'Blue', 'line_id'] = 1
    schedule_car_length.loc[schedule_car_length.route == 'Brn', 'line_id'] = 3
    schedule_car_length.loc[schedule_car_length.route == 'G', 'line_id'] = 4
    schedule_car_length.loc[schedule_car_length.route == 'Org', 'line_id'] = 5
    schedule_car_length.loc[schedule_car_length.route == 'P', 'line_id'] = 7
    schedule_car_length.loc[schedule_car_length.route == 'Red', 'line_id'] = 8
    schedule_car_length.loc[schedule_car_length.route == 'Pink', 'line_id'] = 6
    schedule_car_length.loc[schedule_car_length.route == 'Y', 'line_id'] = 9

    schedule_car_length=schedule_car_length[['car_length','schd_tripstart','line_id']]

    schedule_car_length=schedule_car_length.groupby(['schd_tripstart','line_id']).mean().reset_index()
    schedule_car_length=schedule_car_length.sort_values(['line_id','schd_tripstart','car_length'])
    schedule_car_length['capacity']=(schedule_car_length['car_length']*100).astype(int)

    start_stations=start_stations.merge(schedule_car_length,left_on=['departure_time','line_id'],
                                        right_on=['schd_tripstart','line_id'],how='left')
    start_stations=start_stations.fillna(axis=0,method='ffill')
    start_stations['capacity']=start_stations['capacity'].fillna(400)

    start_stations.loc[:,['trip_id','station_id','line_id','direction_id','capacity','departure_time']]\
        .to_csv('data/startTrainInfo.csv',index=False,header=False)


    #transfer time
    transfer_stations=transfers.drop(columns=['is_transfer'])
    transfer_stations=transfer_stations.merge(stations[['stop_id','line_id','parent_station','is_transfer']],
                                      left_on=['from_stop_id'],right_on=['stop_id'],how='left')
    transfer_stations = transfer_stations.merge(stations[['stop_id', 'line_id', 'parent_station', 'is_transfer']],
                                                 left_on=['to_stop_id'], right_on=['stop_id'], how='left')


    transfer_stations = transfer_stations.merge(encode_part2[['stop_id', 'line_id', 'station_id']],
                                          left_on=['stop_id_x', 'line_id_x'], right_on=['stop_id', 'line_id'],
                                                how='left', validate="m:1")
    transfer_stations=transfer_stations.drop(columns=['stop_id','line_id'])
    transfer_stations = transfer_stations.merge(encode_part1[['parent_station', 'line_id', 'station_id']],
                                          left_on=['parent_station_x', 'line_id_x'], right_on=['parent_station', 'line_id'],
                                                how='left', validate="m:1")
    transfer_stations=transfer_stations.drop(columns=['parent_station','line_id'])
    transfer_stations.loc[transfer_stations['station_id_x'].isna(), 'station_id_x'] \
        = transfer_stations.loc[transfer_stations['station_id_x'].isna(), 'station_id_y']
    transfer_stations = transfer_stations.drop(columns=['station_id_y']).rename(columns={'station_id_x': 'from_station_id'})

    transfer_stations = transfer_stations.merge(encode_part2[['stop_id', 'line_id', 'station_id']],
                                                left_on=['stop_id_y', 'line_id_y'], right_on=['stop_id', 'line_id'],
                                                how='left', validate="m:1")
    transfer_stations = transfer_stations.drop(columns=['stop_id', 'line_id'])
    transfer_stations = transfer_stations.merge(encode_part1[['parent_station', 'line_id', 'station_id']],
                                                left_on=['parent_station_y', 'line_id_y'],
                                                right_on=['parent_station', 'line_id'],
                                                how='left', validate="m:1")
    transfer_stations = transfer_stations.drop(columns=['parent_station', 'line_id'])
    transfer_stations.loc[transfer_stations['station_id_x'].isna(), 'station_id_x'] \
        = transfer_stations.loc[transfer_stations['station_id_x'].isna(), 'station_id_y']
    transfer_stations = transfer_stations.drop(columns=['station_id_y']).rename(
        columns={'station_id_x': 'to_station_id'})

    transfer_link=transfer_stations.loc[transfer_stations['from_station_id']!=transfer_stations['to_station_id'],
                                        ['from_station_id','to_station_id']]
    transfer_link=transfer_link.drop_duplicates().sort_values(['from_station_id','to_station_id'])
    transfer_link['time']=30
    ####################################
    #should add (for one parent station)
    transfer_to_add=pd.concat([encode_part1.loc[encode_part1.is_transfer==True],
                               encode_part2.loc[encode_part2.is_transfer==True,['parent_station','line_id','is_terminal0','is_terminal1','is_transfer','station_id']]])
    transfer_to_add=transfer_to_add.reset_index(drop=True)
    transfer_to_add=pd.pivot(transfer_to_add,index='parent_station',columns='station_id',values='station_id')
    transfer_to_add=transfer_to_add.dropna(thresh=2)

    #transfer_link_copy=transfer_link.copy()
    transfer_to_add.apply(add_parent_transfer,axis=1)
    transfer_link=pd.concat([transfer_link,pd.DataFrame(data={'from_station_id':[148,149,33,33],
                                                              'to_station_id':[33,33,148,149],'time':[30,30,30,30]})])
    transfer_link=transfer_link.drop_duplicates().sort_values(['from_station_id','to_station_id'])


    #update is_transfer
    is_transfer_stations=pd.concat([transfer_link[['from_station_id']],transfer_link[['to_station_id']]])
    is_transfer_stations=is_transfer_stations.fillna(axis=1,method='ffill')
    is_transfer_stations=is_transfer_stations.drop(columns=['from_station_id'])\
        .rename(columns={'to_station_id':'station_id'}).drop_duplicates()
    #is_transfer_stations=pd.concat([is_transfer_stations,pd.DataFrame(data={'station_id':[33]})])
    is_transfer_stations['is_transfer']=True

    stations=stations.drop(columns=['is_transfer']).merge(is_transfer_stations,on=['station_id'],how='left')
    stations=stations.fillna(False)

    parent_stations = stations.loc[:, ['parent_station', 'line_id', 'is_terminal0', 'is_terminal1', 'is_transfer']] \
        .drop_duplicates().reset_index(drop=True)
    parent_stations = parent_stations[
        ~((parent_stations['parent_station'] == 41160) & (parent_stations['is_transfer'] == False))]
    parent_stations['station_id'] = parent_stations.index

    encode_part1 = parent_stations.loc[(parent_stations.line_id == 1) | (parent_stations.line_id == 4) |
                                       (parent_stations.line_id == 8)]
    encode_part1 = encode_part1.sort_values(['parent_station', 'line_id']).reset_index(drop=True)
    encode_part1['station_id'] = encode_part1.index

    encode_part2 = stations.loc[(stations.line_id != 1) & (stations.line_id != 4) &
                                (stations.line_id != 8)].reset_index(drop=True)
    encode_part2['station_id'] = encode_part2.index + 96
    encode_part2['is_dup'] = encode_part2.duplicated(['parent_station', 'line_id'], keep=False)
    encode_part2.loc[encode_part2.is_dup == True, 'is_transfer'] = True

    encoded_stations = pd.concat(
        [encode_part1.loc[:, ['station_id', 'line_id', 'is_terminal0', 'is_terminal1', 'is_transfer']],
         encode_part2.loc[:, ['station_id', 'line_id', 'is_terminal0', 'is_terminal1', 'is_transfer']]],
        ignore_index=True)
    encoded_stations.to_csv('data/stations.csv',index=False,header=False)

    not_transfer=encoded_stations.loc[encoded_stations.is_transfer==False,['station_id']]
    not_transfer=not_transfer.rename(columns={'station_id':'from_station_id'})
    not_transfer['to_station_id']=not_transfer['from_station_id']
    not_transfer['time']=-1

    transfer_link=pd.concat([transfer_link,not_transfer]).sort_values(['from_station_id','to_station_id'])
    transfer_time=pd.pivot(transfer_link,index='from_station_id',columns='to_station_id',values='time')
    transfer_time=transfer_time.fillna(-1)
    transfer_time.to_csv('data/transferTime.csv',index=False,header=False)

    #link regenerate
    schedule_rail = schedule_rail.merge(encode_part2[['stop_id', 'line_id', 'station_id']],
                                        on=['stop_id','line_id'], how='left', validate="m:1")
    schedule_rail=schedule_rail.merge(encode_part1[['parent_station', 'line_id', 'station_id']],
                                      on=['parent_station','line_id'], how='left', validate="m:1")
    schedule_rail=schedule_rail.fillna(axis=1,method='ffill')
    schedule_rail=schedule_rail.drop(columns=['station_id_x']).rename(columns={'station_id_y':'station_id'})
    schedule_rail_shift = schedule_rail.loc[:,
                          ['stop_id', 'trip_id', 'stop_sequence', 'departure_time', 'parent_station','station_id']].shift(
        -1).reset_index()
    schedule_rail_shift['stop_sequence_shift'] = schedule_rail_shift['stop_sequence'] - 1
    link_info = schedule_rail.merge(schedule_rail_shift, left_on=['index', 'trip_id', 'stop_sequence'],
                                    right_on=['index', 'trip_id', 'stop_sequence_shift'])

    link_info['travel_time'] = link_info['departure_time_y'] - link_info['departure_time_x']
    link_info = link_info.rename(columns={'station_id_x': 'link_start', 'station_id_y': 'link_end',
                                          'parent_station_x': 'link_start_parent',
                                          'parent_station_y': 'link_end_parent'})
    link_info = link_info.loc[:, ['link_start', 'link_end', 'travel_time', 'line_id', 'direction_id',
                                  'link_start_parent', 'link_end_parent']].drop_duplicates()
    link_info['travel_time']=link_info['travel_time'].astype(float)
    # use the avg travel time
    link_info = link_info.groupby(['link_start', 'link_end', 'line_id', 'direction_id',
                                   'link_start_parent', 'link_end_parent']).mean().reset_index()
    encoded_link=link_info[['link_start','link_end','travel_time']]\
        .rename(columns={'link_start':'from_station_id','link_end':'to_station_id','travel_time':'time'})\
        .drop_duplicates()
    # encoded_link=link_info.merge(encode_part2[['stop_id', 'line_id', 'station_id']],
    #                                       left_on=['link_start', 'line_id'], right_on=['stop_id', 'line_id'],
    #                                             how='left', validate="m:1")
    # encoded_link=encoded_link.drop(columns=['stop_id'])
    # encoded_link=encoded_link.merge(encode_part1[['parent_station', 'line_id', 'station_id']],
    #                                             left_on=['link_start_parent', 'line_id'],
    #                                             right_on=['parent_station', 'line_id'],
    #                                             how='left', validate="m:1")
    # encoded_link = encoded_link.drop(columns=['parent_station'])
    # encoded_link=encoded_link.fillna(axis=1,method='ffill')
    # encoded_link=encoded_link.drop(columns=['station_id_x'])
    # encoded_link = encoded_link.rename(columns={'station_id_y': 'from_station_id'})
    #
    # encoded_link = encoded_link.merge(encode_part2[['stop_id', 'line_id', 'station_id']],
    #                                left_on=['link_end', 'line_id'], right_on=['stop_id', 'line_id'],
    #                                how='left', validate="m:1")
    # encoded_link = encoded_link.drop(columns=['stop_id'])
    # encoded_link = encoded_link.merge(encode_part1[['parent_station', 'line_id', 'station_id']],
    #                                   left_on=['link_end_parent', 'line_id'],
    #                                   right_on=['parent_station', 'line_id'],
    #                                   how='left', validate="m:1")
    # encoded_link = encoded_link.drop(columns=['parent_station'])
    # encoded_link = encoded_link.fillna(axis=1, method='ffill')
    # encoded_link = encoded_link.drop(columns=['station_id_x'])
    # encoded_link = encoded_link.rename(columns={'station_id_y': 'to_station_id','travel_time':'time'})
    #
    link_info.loc[:,['link_start','link_end','direction_id']].to_csv('data/directions.csv',index=False,header=False)

    #check encoded station sequence
    encoded_sequence=station_sequence.merge(encode_part2[['stop_id', 'line_id', 'station_id']],
                                   on=['stop_id', 'line_id'],how='left', validate="m:1")
    encoded_sequence=encoded_sequence.merge(encode_part1[['parent_station', 'line_id', 'station_id']],
                                            on=['parent_station', 'line_id'],how='left', validate="m:1")
    encoded_sequence = encoded_sequence.fillna(axis=1, method='ffill')
    encoded_sequence = encoded_sequence.drop(columns=['station_id_x'])
    encoded_sequence = encoded_sequence.rename(columns={'station_id_y': 'station_id'})

    #network
    edges=pd.concat([encoded_link.loc[:,['from_station_id','to_station_id','time']],
                     transfer_link.loc[transfer_link.time!=-1]]).sort_values(['from_station_id','to_station_id'])
    G=nx.from_pandas_edgelist(edges,'from_station_id','to_station_id','time', create_using=nx.DiGraph())

    #shortest path
    paths=nx.shortest_path(G,weight='time')
    df_paths=pd.DataFrame.from_dict(paths,orient='index')
    #nx.draw(G, with_labels=True, font_weight='bold')

    #all paths###################???
    all_paths=dict(nx.all_pairs_dijkstra_path(G))
    all_paths=pd.DataFrame.from_dict(all_paths,orient='index')

    #check linkage
    # print(paths[1][66])
    # print(paths[254][255])
    # print(paths[255][254])
    # print(paths[8][218])
    # print(paths[82][218])
    # print(paths[219][82])
    # print(paths[0][1])

    #policy
    next_stations=df_paths.applymap(lambda x:x[1] if len(x)!=1 else -1)
    next_stations=next_stations.stack()
    #print(next_stations.index[0][0])
    #next_stations['from']=next_stations.index.apply(lambda x:x[0])
    next_stations=next_stations.reset_index()
    next_stations=next_stations.rename(columns={'level_0':'from','level_1':'to',0:'next'})
    next_stations=next_stations.loc[next_stations.next!=-1]

    next_stations.to_csv('data/policy.csv',index=False,header=False)

    #policy number
    next_stations['num']=1
    policy_num=pd.pivot(next_stations,index='from',columns='to',values='num')
    policy_num=policy_num.fillna(0)
    policy_num.to_csv('data/policy_num.csv',index=False,header=False)

    #fixed OD
    ODX=pd.read_csv("2019_Oct_10_odx.csv",sep=',',header=0)

    #check type, format
    ODX['boarding_stop']=ODX['boarding_stop'].astype(str)
    ODX=ODX[ODX['boarding_stop'].str.len()==5]
    ODX['boarding_stop']=ODX['boarding_stop'].astype(int)
    ODX=ODX[(ODX['boarding_stop']>=30000)&(ODX['boarding_stop']<50000)
            &(ODX['alighting_stop']>=30000)&(ODX['alighting_stop']<50000)]
    ODX=ODX.rename(columns={'boarding_stop':'from_parent','alighting_stop':'to_stop'})
    ODX['time']=ODX['time'].astype(str)
    ODX['time']=ODX['time'].str.split()
    ODX['time'] = ODX['time'].apply(lambda x:x[1])
    ODX['time'] = ODX['time'].str.split(':').apply(lambda x:int(x[0])*3600+int(x[1])*60)
    ODX_origin=ODX.copy()

    # ODX['route']=ODX['route'].str.split('|')
    #
    # ODX_part1=ODX[ODX['route'].str.len()==1]
    # ODX_part1 = ODX_part1.merge(stations[['parent_station', 'stop_id']],
    #                             left_on=['to'], right_on=['stop_id'], how='left', validate='m:1')
    #
    # ODX_part1=ODX_part1.merge(encode_part1[['parent_station','station_id']],
    #                          left_on=['from'],right_on=['parent_station'],how='left',validate='m:1')

    # ODX_part2=ODX[ODX['route'].str.len()==2]
    # ODX_part2['line_id']=ODX_part2['route'].apply(lambda x:x[1])
    # ODX_part2.loc[ODX_part2['line_id']=='Blue','line_id']=1
    # ODX_part2.loc[ODX_part2['line_id']=='Brn', 'line_id'] = 3
    # ODX_part2.loc[ODX_part2['line_id']=='G', 'line_id'] = 4
    # ODX_part2.loc[ODX_part2['line_id']=='Org', 'line_id'] = 5
    # ODX_part2.loc[ODX_part2['line_id']=='P', 'line_id'] = 7
    # ODX_part2.loc[ODX_part2['line_id']=='Red', 'line_id'] = 8
    # ODX_part2.loc[ODX_part2['line_id']=='Pink', 'line_id'] = 6
    # ODX_part2.loc[ODX_part2['line_id']=='Y', 'line_id'] = 9

    #ODX_part2=ODX_part2.merge(encode_part1[['parent_station','line_id','station_id']],
                              #left_on=['from','line_id'],right_on=['parent_station','line_id'],
                              #how='left',validate='m:1')
    #ODX_part2 = ODX_part2.merge(encode_part2[['parent_station', 'line_id', 'station_id']],
                                #left_on=['from', 'line_id'], right_on=['parent_station', 'line_id'],
                                #how='left')
    #ODX_part2=ODX_part2.drop(columns=['route'])

    ODX=ODX.merge(stations[['parent_station','line_id','station_id']],
                  left_on=['from_parent'],right_on=['parent_station'],how='left')
    ODX = ODX.merge(stations[['stop_id', 'line_id', 'station_id']],
                    left_on=['to_stop'], right_on=['stop_id'], how='left')
    ODX=ODX.drop(columns=['parent_station','stop_id'])
    ODX=ODX.rename(columns={'station_id_x':'from','station_id_y':'to'}).drop_duplicates()
    ODX=ODX.merge(next_stations[['from','to','next']],on=['from','to'],how='left')
    ODX=ODX.merge(stations[['parent_station','station_id']],left_on=['next'],right_on=['station_id'],how='left').drop_duplicates()
    ODX=ODX.drop(columns=['station_id']).rename(columns={'parent_station':'next_parent'})

    #OD without first transfer
    ODX_trim=ODX.loc[ODX['from_parent']!=ODX['next_parent']]
    ODX_trim['is_dup']=ODX_trim.duplicated(subset=['from_parent','to_stop','time'])
    ODX_trim=ODX_trim.loc[ODX_trim['is_dup']==False]

    OD=ODX_origin[['from_parent','to_stop','time','group_size']].groupby(['from_parent','to_stop','time']).sum().reset_index()
    OD=OD.merge(ODX_trim[['from_parent','to_stop','from','to','time']],on=['from_parent','to_stop','time'])
    OD=OD.sort_values(['time'])

    #OD[['from','to','group_size','time']].to_csv('data/fixedOD.csv',index=False,header=False)

    #stations.to_csv("check_stations.csv")

    ###### special edition without part of purple line(7)

    special_list=[192,148,	233,	219	,147,	223,	157	,179	,167,	141,	130,	120	,251,	161	,187	,113	,98	,170,	139,	165	,177,	155	,221	,145	,217	,231	,149]

    encoded_link2=encoded_link.loc[~(encoded_link['from_station_id'].isin(special_list))]

    edges2 = pd.concat([encoded_link2.loc[:, ['from_station_id', 'to_station_id', 'time']],
                       transfer_link.loc[transfer_link.time != -1]]).sort_values(['from_station_id', 'to_station_id'])
    G2 = nx.from_pandas_edgelist(edges2, 'from_station_id', 'to_station_id', 'time', create_using=nx.DiGraph())

    # shortest path 2
    paths2 = nx.shortest_path(G2, weight='time')
    df_paths2 = pd.DataFrame.from_dict(paths2, orient='index')
    df_paths2=df_paths2.fillna('n')

    # policy2
    next_stations2 = df_paths2.applymap(lambda x: x[1] if len(x) > 1 else -1)
    next_stations2 = next_stations2.stack()
    # print(next_stations.index[0][0])
    # next_stations['from']=next_stations.index.apply(lambda x:x[0])
    next_stations2 = next_stations2.reset_index()
    next_stations2 = next_stations2.rename(columns={'level_0': 'from', 'level_1': 'to', 0: 'next'})
    next_stations2 = next_stations2.loc[next_stations2.next != -1]

    next_stations2.to_csv('data/policy2.csv', index=False, header=False)

    #check OD for deleted stations
    deleted_stations=[96,97,98,113,251]
    problem=OD.loc[(OD['to'].isin(deleted_stations)) | (OD['from'].isin(deleted_stations))]

