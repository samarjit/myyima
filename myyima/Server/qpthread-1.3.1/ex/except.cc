#include "ex.h"
#include <qpthr/qp.h>

__USE_QPTHR_NAMESPACE

class MyException: public QpUserException 
{
    public:
	MyException(int code = 0, const char *text = NULL): QpUserException(code, text) {} 
	virtual QpClonedException *Clone() const {
		return new MyException(e_code, e_text);
	}
	virtual void Raise() const { throw *this;}
};


class HelloWorld: public QpThread
{
	public:
		virtual void Main() {
			throw MyException();
		}
};

int main()
{
	QpInit init;
	HelloWorld hw;

 	hw.Start();
 	hw.Join();
	try {
 		hw.Raise();
	}
 	catch (MyException &e) {
		cout << "MyException has been thrown" << endl;
	}
	catch (QpClonedException &e) {
		cout << "ClonedException has been thrown" << endl;
	}
}
