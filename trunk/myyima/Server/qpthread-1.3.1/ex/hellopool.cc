#include "ex.h"
#include <qpthr/qp.h>
#include <qpthr/qp_work.h>

__USE_QPTHR_NAMESPACE

void HelloWorld()
{
	cout << "Hello World" << endl;
}

class HW
{
    private:
	int m_i;
    public:
	HW(int i): m_i(i) {}
	void f() {
		cout << "HW::f() " << m_i << endl;
	}
};

int main()
{
	QpInit qp_init;
	QpThreadPool tpool;

	QpWork_p0 *w = new QpWork_p0(&HelloWorld);
	tpool.Do(w);
	tpool.Wait();
	delete w;
	
	HW hw(0);
	QpWork_mp0<HW> *wp = new QpWork_mp0<HW>(hw, &HW::f);
	tpool.Do(wp);
	tpool.Wait();
	delete wp;

	const HW chw(1);
	QpWork_mp0<HW> *cwp = new QpWork_mp0<HW>(chw, &HW::f);
	tpool.Do(cwp);
	tpool.Wait();
	delete cwp;

	return 0;
}

