#pragma once


class Ch_IEngine
{
public:
	virtual void Init( const std::vector< std::string >& srModulePaths ) = 0;

	virtual void Update( float sDT ) = 0;

	virtual bool IsActive() = 0;
};

