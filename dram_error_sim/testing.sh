#rm *.i6.1e-1.1e-5.1e-7

#12341234 for detailed
#1234 for coarse grained
#get arguemnts about detail or coarse grained

if [ $# -eq 0 ]
then
    echo "No arguments supplied"
    exit 1
fi

# if input is detailed, idx set to 12341234
# if input is coarse grained, idx set to 1234
if [ $1 = "detailed" ]; then
    idx=12351234
else
    idx=1234
fi

 
for i in 4020 4021 4030 4031 4040 4041 #4000 4001 4002 4010 4011 4012 4013 4020
#1122 1123 352 353 354 355 362 333 334 335
# 330 331 332 333 334 335 350 351 352 353 354 355 360 361 362
#301 302 303 304 310 311 312 313 314 320 321 322 323 324 330 331 920 921 940 941 970 971
do
./faulterrorsim ${i} 100000000 ${idx} S i6 0 1e-5 1e-5 > results/${i}_${idx}_output_1e-5_1e-5.txt &
#./faulterrorsim ${i} 100000000 ${idx} S i6 0 1e-5 1e-7> results/${i}_${idx}_output_1e-5_1e-7.txt & 
done

wait


