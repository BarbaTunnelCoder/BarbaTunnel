#pragma once

/*
 * @class A wrapper-class around Critical Section functionality, WIN32 & PTHREADS.
 */
class SimpleCriticalSection
{
public:
    /**
     * @brief SimpleCriticalSection class constructor.
     */
    explicit SimpleCriticalSection(void)
    {
    #ifdef _WIN32
        if (0 == InitializeCriticalSectionAndSpinCount(&m_cSection, 0))
            throw("Could not create a SimpleCriticalSection");
    #else
        if (pthread_mutex_init(&m_cSection, NULL) != 0)
            throw("Could not create a SimpleCriticalSection");
    #endif
    }; // SimpleCriticalSection()
 
    /**
     * @brief SimpleCriticalSection class destructor
     */
    ~SimpleCriticalSection(void)
    {
        Enter(); // acquire ownership (for pthread)
    #ifdef _WIN32
        DeleteCriticalSection(&m_cSection);
    #else
        pthread_mutex_destroy(&m_cSection);
    #endif
    }; // ~SimpleCriticalSection()
 
    /**
     * @fn void Enter(void)
     * @brief Wait for unlock and enter the SimpleCriticalSection object.
     * @see TryEnter()
     * @return void
     */
    void Enter(void)
    {
    #ifdef _WIN32
        EnterCriticalSection(&m_cSection);
    #else
        pthread_mutex_lock(&m_cSection);
    #endif
    }; // Enter()
 
    /**
     * @fn void Leave(void)
     * @brief Leaves the critical section object.
     * This function will only work if the current thread
     * holds the current lock on the SimpleCriticalSection object
     * called by Enter()
     * @see Enter()
     * @return void
     */
    void Leave(void)
    {
    #ifdef _WIN32
        LeaveCriticalSection(&m_cSection);
    #else
        pthread_mutex_unlock(&m_cSection);
    #endif
    }; // Leave()
 
    /**
     * @fn bool TryEnter(void)
     * @brief Attempt to enter the SimpleCriticalSection object
     * @return bool(true) on success, bool(false) if otherwise
     */
    bool TryEnter(void)
    {
        // Attempt to acquire ownership:
    #ifdef _WIN32
        return(TRUE == TryEnterCriticalSection(&m_cSection));
    #else
        return(0 == pthread_mutex_trylock(&m_cSection));
    #endif
    }; // TryEnter()
 
private:
#ifdef _WIN32
    CRITICAL_SECTION m_cSection; //!< internal system critical section object (windows)
#else
    pthread_mutex_t m_cSection; //!< internal system critical section object (*nix)
#endif
};

//SimpleLock
class SimpleLock
{
private:
	SimpleCriticalSection* _CriticalSection;
	bool _IsLocked;
public:
	explicit SimpleLock(SimpleCriticalSection* cs, bool lock=true)
	{
		_IsLocked = false;
		_CriticalSection = cs;
		if (lock)
			Lock();
	}

	~SimpleLock()
	{
		Unlock();
	}

	void Lock()
	{
		_IsLocked = true;
		_CriticalSection->Enter();
	}

	void Unlock()
	{
		if (_IsLocked)
		{
			_CriticalSection->Leave();
			_IsLocked = false;
		}
	}
};

