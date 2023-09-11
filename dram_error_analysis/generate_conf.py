import pandas as pd

# Load the CSV file into a DataFrame
df = pd.read_csv('feature_vector_df_nosocket.csv')

df['failure_classification'] = df['failure_type'].apply(lambda x: 'Uncorrectable' if x == 'Uncorrectable' else 'Not Uncorrectable')

# Group by 'category', 'permanency', and 'failure_classification' and count the occurrences
grouped_counts_correctability = df.groupby(['category', 'permanency', 'failure_classification']).size().reset_index(name='count')

# Calculate the percentage for each combination
grouped_counts_correctability['percentage'] = (grouped_counts_correctability['count'] / df.shape[0])

# calculate FIT rate based on input about how many DIMMs exist and how many hours the DIMMs have been running
num_DIMMs = 3000000
hours = 5856

#per 1 rank, 18 chips 
FITrate = len(df)/num_DIMMs/18/hours*1000000000

grouped_counts_correctability['FIT'] = FITrate * grouped_counts_correctability['percentage']

# Save the DataFrame to a CSV file
grouped_counts_correctability.to_csv('FITrate.csv')

param_distribution = pd.read_pickle("param_distribution.pkl")

param_to_number = {
    'numofBanks': 16,
    'CDEC_per_chip': 16,
    'WLD_per_chip': 10240,
    'RDEC_per_chip': 16,
    'BITS_per_chip': 1,
    'BLSA_per_chip': 4194304,
    'CSL_per_chip': 256,
    'RDEC_SUBBANK_per_chip': 16,
    1: 1
}

df_csv = pd.read_csv("FITrate.csv")

param_mapping = param_distribution.to_dict()

# Define a function to get the 'param' value based on the mapping
def get_param(row):
    key = (row['category'], row['permanency'], row['failure_classification'])
    return param_mapping.get(key, None)

# Apply the function to the CSV DataFrame to get the 'param' values
df_csv['param'] = df_csv.apply(get_param, axis=1)
df_csv['param'] = df_csv['param'].apply(lambda x: x[0] if x else None)
df_csv['param'] = df_csv['param'].apply(lambda x: param_to_number[x] if x else None)

# Calculate FIT/param
df_csv['FIT_per_param'] = df_csv['FIT'] / df_csv['param']

# Create a new column with the desired format "{category}-{p/t}"
df_csv['category_pt'] = df_csv['category'] + "-" + df_csv['permanency'].str[0].str.lower()

# Filter the required columns and format the output
output = df_csv[['category_pt', 'FIT_per_param']]

output_values = output.values.tolist()

formatted_output = [(f'"{x[0]}",{x[1]:.6e}') for x in output_values]
# Save the output to a file
with open("input_FIT.conf", "w") as f:
    f.write("\n".join(formatted_output))