#include "ex.h"
#include <qpthr/qp.h>

__USE_QPTHR_NAMESPACE

class HelloWorldRun: public QpRunnable {
    public:
	virtual void Main() {
		cout << "Hello World" << endl;
	}
};


int main()
{
	QpInit qp_init;
	HelloWorldRun hw;
	QpThread t(&hw);
	
	t.Start();
	t.Join();
	return 0;
}

