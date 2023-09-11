#include <vector>

/* code from https://ideone.com/86h2wn */

//T== int K== vector<int>
template <typename T, typename K>
void Combination(const std::vector<T>& v, std::size_t count,std::vector<K>& output)
{
    if(count > v.size()){
        Combination<T,K>(v, count-1, output);
    }
    std::vector<bool> bitset(v.size() - count, 0);
    bitset.resize(v.size(), 1);

    do {
        K tmp;
        int idx = 0;
        for (std::size_t i = 0; i != v.size(); ++i) {
            if (bitset[i]) {
                tmp.push_back(v[i]);
            }
        }
        output.push_back(tmp);
    } while (std::next_permutation(bitset.begin(), bitset.end()));
}

template <typename T, typename K>
void All_Combination(const std::vector<T>& v, std::vector<K>& output)
{
    for(int i =1;i<=v.size();i++)
    {
        Combination<T,K>(v, i, output);
    }
}