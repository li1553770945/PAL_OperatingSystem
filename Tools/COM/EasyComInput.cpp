#include <bits/stdc++.h>
using namespace std;

int main(int argc,char **argv)
{
	if (argc!=2)
		return cerr<<"ttyS* number not specified!"<<endl,1;
	string tty=string("/dev/ttyS")+argv[1];
	cout<<"CommandLine start:"<<endl;
	while (1)
	{
		cout<<":";
		string cmd;
		getline(cin,cmd);
		if (cmd=="qqq"||cmd=="QQQ")
			break;
		system(("echo \""+cmd+"\" > "+tty).c_str());
	}
	return 0;
}
