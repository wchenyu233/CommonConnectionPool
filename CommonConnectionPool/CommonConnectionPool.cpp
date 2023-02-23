#include "CommonConnectionPool.h"

//连接池的构造
ConnectionPool::ConnectionPool()
{
	//加载配置项
	if (!loadConfigFile())
	{
		return;
	}

	//创建初始数量的连接
	for (int i = 0; i < _initSize; ++i)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refreshAliveTime();//刷新一下开始空闲的时间
		_connectionQue.push(p);
		_connectionCnt++;
	}

	//启动一个新线程，作为生产者
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
	produce.detach();//分离线程

	//启动一个新的定时线程,扫描超过maxIdleTime空闲连接，进行连接回收
	thread scanner(std::bind(&ConnectionPool::scannerConnetionTask, this));
	scanner.detach();
}

//运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty())
		{
			cv.wait(lock); //队列不空，生产线程进入等待状态(两步操作，第一步释放锁，第二步进入等待状态)
		}

		//连接数量没有到达上限，继续创建新的连接
		if (_connectionCnt < _maxSize)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime();
			_connectionQue.push(p);
			_connectionCnt++;
		}

		//通知消费者线程，可用消费连接了
		cv.notify_all();
	}
}

//扫描超过maxIdleTime空闲连接，进行连接回收
void ConnectionPool::scannerConnetionTask()
{
	for (;;)
	{
		//通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		//扫描整个队列，释放多余的连接
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() > (_maxIdleTime * 1000))
			{
				_connectionQue.pop();
				_connectionCnt++;
				delete p;//释放连接  调用析构函数
			}
			else
				break; //队头没超过，其它也不会超过
		}
	}
}

//给外部提供接口，从连接池钟获取可用空闲连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while(_connectionQue.empty())
	{
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			if (_connectionQue.empty())
			{
				LOG("获取空闲连接超时了...")
					return nullptr;
			}
		}
	}

	//需要自定义shared_ptr的删除器
	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection* pcon) {
			// 在服务器应用线程中调用，需要考虑线程安全
			unique_lock<mutex> lock(_queueMutex);
			pcon->refreshAliveTime();
			_connectionQue.push(pcon);
		}
	);
	_connectionQue.pop();
	cv.notify_all();//消费完连接以后，通知生产者生产
	return sp;
}

// 线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool; // lock和unlock
	//声明了构造函数却未实现会导致链接错误
	return &pool;
}

 //从配置文件中加载配置项
bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr)
	{
		LOG("mysql.ini file is not exist!");
		return false;
	}

	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=', 0);
		if (idx == -1) // 无效的配置项
		{
			continue;
		}

		int endidx = str.find('\n', idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);

		if (key == "ip")
			_ip = value;
		else if (key == "port")
			_port = atoi(value.c_str());
		else if (key == "username")
			_username = value;
		else if (key == "password")
			_password = value;
		else if (key == "dbname")
			_dbname = value;
		else if (key == "initSize")
			_initSize = atoi(value.c_str());
		else if (key == "maxSize")
			_maxSize = atoi(value.c_str());
		else if (key == "maxIdleTime")
			_maxIdleTime = atoi(value.c_str());
		else if (key == "connectionTimeOut")
			_connectionTimeout = atoi(value.c_str());
	}
	return true;
}


