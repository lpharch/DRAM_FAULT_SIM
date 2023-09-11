import pandas as pd
import numpy as np
import pickle
import dask.dataframe as dd

from utils import groupby_machine_informations, make_decision, generate_min_max

def load_csv(filename):
    '''Load csv and return a dataframe.'''
    df = dd.read_csv(filename, dtype={'sid': 'str', 'memoryid': 'int64', 'rankid': 'int64', 'bankid': 'int64', 'row': 'int64', 'col': 'int64', 'error_type': 'int64', 'error_time': 'str'})
    return df

if __name__ == '__main__':
    trouble_tickets = pd.read_csv('trouble_tickets.csv')
    trouble_tickets.to_pickle('trouble_tickets.pkl')
    df = load_csv('mcelog.csv')
    date = df['error_time'].apply(lambda x: '-'.join(x.split('-')[:3]), meta=('error_time', 'str'))

    date = date.apply(lambda x: x.replace('0001-01', '2019-10'), meta=('error_time', 'str'))
    date = date.apply(lambda x: x.replace('0001-02', '2019-11'), meta=('error_time', 'str'))
    date = date.apply(lambda x: x.replace('0001-03', '2019-12'), meta=('error_time', 'str'))
    date = date.apply(lambda x: x.replace('0001-04', '2020-01'), meta=('error_time', 'str'))
    date = date.apply(lambda x: x.replace('0001-05', '2020-02'), meta=('error_time', 'str'))
    date = date.apply(lambda x: x.replace('0001-06', '2020-03'), meta=('error_time', 'str'))
    date = date.apply(lambda x: x.replace('0001-07', '2020-04'), meta=('error_time', 'str'))
    date = date.apply(lambda x: x.replace('0001-08', '2020-05'), meta=('error_time', 'str'))

    df['error_time'] = date
    df['error_time'] = dd.to_datetime(df['error_time'])

    try:
        with open('data_df.pkl', 'rb') as f:
            df = pickle.load(f)
    except:
        df = df.compute()
        with open('data_df.pkl', 'wb') as f:
            pickle.dump(df, f)

    try:
        transient_phy_res = pickle.load(open('transient_phy_res.pkl', 'rb'))
        permanent_phy_res = pickle.load(open('permanent_phy_res.pkl', 'rb'))
    except:       
        df = df.sort_values(by=['sid', 'memoryid', 'rankid', 'bankid', 'row', 'col','error_time']).reset_index(drop=True)
        df = generate_min_max(df).reset_index(drop=True)
        df = groupby_machine_informations(df)
        
        min_value = df.groupby(['sid','memoryid'])['error_time_min'].apply(lambda x: x.min())
        max_value = df.groupby(['sid','memoryid'])['error_time_max'].apply(lambda x: x.max())
        diff_value = ((max_value - min_value) < pd.Timedelta(24,'h')).reset_index()
        diff_value.columns = ['sid','memoryid','diff_max_min']
        diff_df = df.merge(diff_value, on=['sid','memoryid'], how ='left')
        transient_df = diff_df[diff_df['diff_max_min'] == True].reset_index(drop=True)
        might_permanent_df = diff_df[diff_df['diff_max_min'] == False].reset_index(drop=True)
        transient_df = transient_df.drop(columns='diff_max_min')
        might_permanent_df = might_permanent_df.drop(columns='diff_max_min')

        ifeature_vector_df = transient_df.groupby(['sid', 'memoryid']).agg({'rankid': list, 'bankid': list, 'row': list, 'col': list, 'error_time_min': list, 'error_time_max': list, 'DRAM_model': list})
        ifeature_vector_df['DRAM_model'] = ifeature_vector_df['DRAM_model'].apply(lambda x: x[0])
        ifeature_vector_df = ifeature_vector_df.reset_index()
        ifeature_vector_df = ifeature_vector_df[(ifeature_vector_df['DRAM_model']!="B2") & (ifeature_vector_df['DRAM_model']!="B3")]

        pfeature_vector_df = might_permanent_df.groupby(['sid', 'memoryid']).agg({'rankid': list, 'bankid': list, 'row': list, 'col': list, 'error_time_min': list, 'error_time_max': list, 'DRAM_model': list})
        pfeature_vector_df['DRAM_model'] = pfeature_vector_df['DRAM_model'].apply(lambda x: x[0])
        pfeature_vector_df = pfeature_vector_df.reset_index()
        pfeature_vector_df = pfeature_vector_df[(pfeature_vector_df['DRAM_model']!="B2") & (pfeature_vector_df['DRAM_model']!="B3")]

        transient_log_res, transient_phy_res = make_decision(ifeature_vector_df)
        permanent_log_res, permanent_phy_res = make_decision(pfeature_vector_df)


    category = pd.DataFrame(columns=['sid', 'memoryid', 'rankid', 'bankid', 'category', 'permanency'])
    for key in permanent_phy_res.keys():
        permanent_phy_res
        if permanent_phy_res[key].empty:
            continue
        category = pd.concat([category, pd.DataFrame({'sid': permanent_phy_res[key]['sid'], 'memoryid': permanent_phy_res[key]['memoryid'], 'category': key, 'permanency': 'permanent'})])
    for key in transient_phy_res.keys():
        if transient_phy_res[key].empty:
            continue
        category = pd.concat([category, pd.DataFrame({'sid': transient_phy_res[key]['sid'], 'memoryid': transient_phy_res[key]['memoryid'], 'category': key, 'permanency': 'transient'})])
    category.reset_index(drop=True, inplace=True)
    with open('category.pkl', 'wb') as f:
        pickle.dump(category, f)
