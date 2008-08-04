struct signalflag
{
    int interrupt;
};

#ifdef SIG_MAIN
struct signalflag signalflag;
#else
extern struct signalflag signalflag;
#endif
