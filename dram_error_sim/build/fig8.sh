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
for module in 0 1 2 4
do
# check if DDR5_results_${module} exists if not, create it
if [ ! -d "fig8" ]; then
  mkdir fig8
fi
for i in 5003 5004 5001 5002 4021 6002 # 4019 4020 4021 4022 4031 4032
do
#if i is 5003 5004 5001 5002, DRAMTYPE="DDR5"
#if i is 4021 HBM3
#if i is 6002 LPDDR5 
if [ $i -eq 5003 ] || [ $i -eq 5004 ] || [ $i -eq 5001 ] || [ $i -eq 5002 ]
then
DRAMTYPE="DDR5"
elif [ $i -eq 4021 ]
then
DRAMTYPE="HBM3"
elif [ $i -eq 6002 ]
then
DRAMTYPE="LPDDR5"
fi

num_samples=100000000

 ./faulterrorsim ${i} ${num_samples} ${idx} S i9 0 1e-5 1e-5 ${module} ${DRAMTYPE} > fig8/${i}_${idx}_output_1e-5_1e-5.txt &
 ./faulterrorsim ${i}  ${num_samples} ${idx} S i9 0 1e-6 1e-5 ${module} ${DRAMTYPE} > fig8/${i}_${idx}_output_1e-6_1e-5.txt & 
./faulterrorsim ${i}  ${num_samples} ${idx} S i9 0 1e-7 1e-5 ${module} ${DRAMTYPE} > fig8/${i}_${idx}_output_1e-7_1e-5.txt &  
./faulterrorsim ${i}  ${num_samples} ${idx} S i9 0 1e-8 1e-5 ${module} ${DRAMTYPE} > fig8/${i}_${idx}_output_1e-8_1e-5.txt &


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


