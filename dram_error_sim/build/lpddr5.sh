#rm *.i6.1e-1.1e-5.1e-7

while true; do
    count=$(pgrep -c faulterrorsim)
    if [ $count -lt 16 ]; then
        echo "Found $count instances of faulterrorsim. Starting execution."
        break
    else
        echo "Found $count instances of faulterrorsim. Waiting for 1 minute before checking again."
        sleep 60
    fi
done
# idx from 0 to 100
for idx in 0
do
for module in 4
do
# check if LPDDR5_results_${module} exists if not, create it
if [ ! -d "LPDDR5_MERGE_results_${module}" ]; then
  mkdir LPDDR5_MERGE_results_${module}
fi
for i in 6002 #6000 6001 6002 # 6000 6001 6002 #5001 5002 4019 4020 4021 4022 4031 4032
do
# if module is 0, num_sample is 494096
# if module is 1, num_sample is 209856
# if module is 2, num_sample is 171152

num_samples=10000000
DRAMTYPE="LPDDR5_MERGE"
 ./faulterrorsim ${i} ${num_samples} ${idx} S i9 0 1e-5 1e-5 ${module} ${DRAMTYPE} > ${DRAMTYPE}_results_${module}/${i}_${idx}_output_1e-5_1e-5.txt &
 ./faulterrorsim ${i}  ${num_samples} ${idx} S i9 0 1e-6 1e-5 ${module} ${DRAMTYPE} > ${DRAMTYPE}_results_${module}/${i}_${idx}_output_1e-6_1e-5.txt & 
./faulterrorsim ${i}  ${num_samples} ${idx} S i9 0 1e-7 1e-5 ${module} ${DRAMTYPE} > ${DRAMTYPE}_results_${module}/${i}_${idx}_output_1e-7_1e-5.txt &  
./faulterrorsim ${i}  ${num_samples} ${idx} S i9 0 1e-8 1e-5 ${module} ${DRAMTYPE} > ${DRAMTYPE}_results_${module}/${i}_${idx}_output_1e-8_1e-5.txt &


#./faulterrorsim ${i} ${num_sample}  ${idx} S i9 0 0 1e-5 ${module} > results_${module}/${i}_${idx}_output_0_0.txt &
done
while true; do
    count=$(pgrep -c faulterrorsim)
    if [ $count -lt 32 ]; then
        echo "Found $count instances of faulterrorsim. Starting execution."
        break
    else
        echo "Found $count instances of faulterrorsim. Waiting for 1 minute before checking again."
        sleep 1
    fi
done
done
done

wait


