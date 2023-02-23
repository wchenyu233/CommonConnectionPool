#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <iostream>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>
#include <ctime>

using namespace std;
#include "Connection.h"

/*
* ʵ�����ӳع���ģ��
*/
class ConnectionPool
{
public:
	//��ȡ���ӳض���ʵ��
	static ConnectionPool* getConnectionPool();
	//���ⲿ�ṩ�ӿڣ������ӳ��ӻ�ȡ���ÿ�������
	shared_ptr<Connection> getConnection();

private:
	//�������ļ��Ӽ���������
	bool loadConfigFile();
	ConnectionPool(); // ����#1 ���캯��˽�л�
	
	//�����ڶ������߳��У�ר�Ÿ�������������
	void produceConnectionTask();

	//ɨ�賬��maxIdleTime�������ӣ��������ӻ���
	void scannerConnetionTask();

	string _ip;
	unsigned short _port;	// mysql��ip��ַ
	string _username;	//mysql�Ķ˿ں�
	string _password;	//mysql��¼����
	int _initSize;		//���ӳصĳ�ʼ������
	int _maxSize;	//���ӳص����������
	int _maxIdleTime;  //���ӳ�������ʱ��
	string _dbname; //���ӵ����ݿ�����
	int _connectionTimeout;	//���ӳػ�ȡ���ӵĳ�ʱʱ��


	queue<Connection*> _connectionQue; //�洢mysql���ӵĶ���
	mutex _queueMutex;	//ά�������̰߳�ȫ�Ļ�����
	atomic_int _connectionCnt; //��¼������������connection������ �̰߳�ȫ
	condition_variable cv; //�������������������̺߳������̵߳�ͨ��
};