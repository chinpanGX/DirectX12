/*------------------------------------------------------------
	
	[Singleton.h]
	Author : �o���đ�

	�V���O���g���e���v���[�g

--------------------------------------------------------------*/
#pragma once

template <typename T>
class Singleton
{
public:
	inline static T& Get()
	{
		static T instance;
		return instance;
	}
protected:
	inline Singleton(){}
	inline virtual ~Singleton(){}
private:
	Singleton(const Singleton&) = delete;
	void operator=(const Singleton &) = delete;
};