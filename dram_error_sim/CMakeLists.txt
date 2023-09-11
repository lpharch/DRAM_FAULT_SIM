ADD_COMPILE_OPTIONS(-g -O3 -std=c++11)# -fsanitize=address)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")# -fsanitize=address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")# -fsanitize=address")

#SET (CMAKE_BUILD_TYPE Debug)
#SET (CMAKE_BUILD_TYPE Release)
set (CMAKE_C_COMPILER g++)
#SET ( CMAKE_C_FLAGS_RELEASE "-O3" )
#SET ( CMAKE_C_FLAGS_DEBUG "-g -O0" )
ADD_EXECUTABLE( faulterrorsim Bamboo.cc
binary_linear_codec.cc
DomainGroup.cc
DUO.cc
ECC.cc
Fault.cc
FaultDomain.cc
FaultRateInfo.cc
gf.cc
hsiao.cc
Huawei.cc
linear_codec.cc
LOT.cc
main.cc
message.cc
MultiECC.cc
prior.cc
REGB.cc
sec.cc
Tester.cc
util.cc
VECC.cc
XED.cc
FlipCorrection.cc
Config.cc
uint128_t.cpp
)
