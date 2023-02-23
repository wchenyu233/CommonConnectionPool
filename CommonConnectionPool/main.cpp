
#include <iostream>
#include "CommonConnectionPool.h"
using namespace std;

int main()
{
	/*char sql[1024] = { 0 };
	sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
			"zhang san", 20, "male");
	conn.connect("127.0.0.1", 3306, "root", "chenyugq", "chat");
	conn.update(sql);*/

	clock_t begin = clock();
	for (int i = 0; i < 1000; ++i)
	{	
		shared_ptr<Connection> sp = cp->getConnection();
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
			"zhang san", 20, "male");
		sp->update(sql);
	}
	clock_t end = clock();
	cout << (end - begin) << "ms" << endl;  
	return 0;
}