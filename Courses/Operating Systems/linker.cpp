#include <iostream>

using namespace std;

int main(int argc, char const *argv[])
{
    if (argc==1){
        cout<<"Missing Argument: Please pass the path to the linker input file.\n";
        exit(1);
    }

    cout<<argv[1];

    return 0;
}

