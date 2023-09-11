import os
import re
import pandas as pd

def get_classification_info(root_dir):
    # create an empty list to hold the classification info
    classification_info = []
    
    # walk through the directory tree recursively
    for dirpath, dirnames, filenames in os.walk(root_dir):
        # check if any files in this directory match the pattern
        matching_files = [f for f in filenames if re.match(r'Server_\d+_\d+', f)]
        
        # if there are matching files, extract the classification info
        if len(matching_files) > 0:
            # get the directory names
            dirnames = os.path.normpath(dirpath).split(os.path.sep)
            
            #DRAM_name = dirnames[-1]
            #dirnames = dirnames[:-1]

            # the top-level directory is the classification
            classification = dirnames[-1]
            
            # the subdirectories are the clusters
            clusters = dirnames[:-1]
            
            # loop through the matching files and extract the info
            for filename in matching_files:
                server_id, memory_id = filename.split('_')[1:]
                memory_id = os.path.splitext(memory_id)[0]  # remove file extension
                info = {'sid': 'Server_'+server_id, 'memoryid': memory_id, 'category': clusters[1], 'Classification_detail': classification}#, 'DRAM': DRAM_name}
                classification_info.append(info)
    
    # create a Pandas DataFrame from the classification info
    df = pd.DataFrame(classification_info)
    # dump into pickle
    df.to_pickle('manual_category.pkl')
    
    
    return df

get_classification_info('kmeans')