#include <QCoreApplication>
#include <QUrl>
#include <QDebug>
#include "messagebus.h"
#include "sayhelloservice.h"
#include "objectproxy.h"
#include "object.h"

int result(QVariantList va)
{
    qDebug() << "Callback result called with args" << va;
    return 0;
}

int testFunc(QString s)
{
    qDebug() << "Testfunc called with:" << s;
    return 37;
}

void sumFound(int sum)
{
    qDebug() << "Sum = " << sum;
}

void gotSomethings(QVariantList list)
{
    qDebug() << "Got some things" << list;
}

template <typename Type = void>
struct Repl
{
    Repl():m_value(),m_isValid(false) {}
    Repl(const Type &value):m_value(value),m_isValid(true) {}

    inline operator Type() const { return m_value; }
    inline Type value() const { return m_value; }

    Repl<Type> &operator =(const Type &value)
    {
        setValue(value);
        return *this;
    }

    inline void setValue(const Type &value)
    {
        m_value = value;
        m_isValid = true;
    }

    inline void clear()
    {
        m_value = Type();
        m_isValid = false;
    }

    inline bool isValid() const { return m_isValid; }

private:
    Type m_value;
    bool m_isValid;
};

template<>
struct Repl<void>
{
    Repl() { }

    inline bool isValid() const { return true; }
};

class A
{
public:
    QString *operator ->()
    {
        return new QString("hello");
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Repl<> r;
    Repl<int> ir(3);
    Repl<QString> is("Hello");
    ir.setValue(3);
    qDebug() << is;

    MessageBus *messageBus = new MessageBus(&a);
    SayHelloService *shs = new SayHelloService(*messageBus);

    /*Service *service = messageBus->getService("com.barco.cirrus.sayhelloservice");
    service->getObject("sayHelloTest");
    PendingReply *penr = messageBus->registerService("com.barco.cirrus.sayhelloservice");
    penr->onReady(function() {
                      service->registerObject("sayHelloTest", sayHelloTest);
                  });*/

    QObject::connect(messageBus, &MessageBus::connected, function(QString) {
                         PendingReply *pr = messageBus->getObject("com.barco.cirrus.sayhalloservice", "sayHelloTest");
                         pr->onFinished(function()
                         {
                             if (pr->isOK())
                             {
                                 Object *pObject = qvariant_cast<Object *>(pr->getResult());
                                 pObject->call("sayHello", "sayHello after object retrieval");
                             }
                             else
                             {
                                 qDebug() << "Error getting object:" << pr->getErrorString();
                             }
                         });
                     });

    QObject::connect(shs, &SayHelloService::registered, function() {
                         PendingReply *pr = messageBus->execute("com.barco.cirrus.sayhelloservice", "sayHelloTest", "sayHello", "Hello from main");
                         pr->onReady(function() {
                             qDebug() << "main(): Received reply:" << pr->getResult().toString();
                         });

                         messageBus->getObject("com.barco.cirrus.sayhelloservice", "sayHelloTest", function(Object *pObject) {
                             qDebug() << "Got object, saying hello";
                             pObject->call("sayHello", "Got Object through callback");
                         });

                         //TODO: add getObject to MessageBus; should only return when the object is available
                         ObjectProxy *objref = new ObjectProxy(*messageBus, "com.barco.cirrus.sayhelloservice", "sayHelloTest");
                         objref->on("doSayHello", function(QString msg) { qDebug() << ">>>>>>>>>>>>>>>>>>>>onDoSayHello:" << msg; });
                         objref->on("doSayHello", testFunc);
                         objref->call("sayHello", "Hello from objectcall", function(QVariant result)
                         {
                             qDebug() << "main(): Got result from objectreference:" << result.toString();
                         });
                         objref->call("add", QVariantList() << 1 << 2, function(QVariant result)
                         {
                             qDebug() << "main(): Got result from objectreference add call:" << result.toInt();
                         });
                         objref->call("add", 4, 48, function(int result)
                         {
                             qDebug() << "main(): Got result from objectreference variadic add call:" << result;
                         });

                         objref->call("sayHello", "Direct call", testFunc);
                         objref->call("sayHello", "No callback");
                         objref->call("getSomeThings", gotSomethings);
                         objref->call("sayHello", "Direct call2", function(QString list)
                         {
                             qDebug() << "Lambda callback:" << list;
                         });

                         ObjectProxy *monitoringref = new ObjectProxy(*messageBus, "com.barco.cirrus.monitoringservice", "cpuMonitor");
                         monitoringref->call("getNumberOfCPUs", function(int result)
                         {
                             qDebug() << "This PC contains" << result << "CPUs";
                         });
                         //monitoringref->on("cpuStatus", shs, SLOT(onCPUStatus(QVariantMap)));
                         monitoringref->on("cpuStatus", function(QVariantMap status) {
                             qDebug() << status;
                         });
                         //monitoringref->on("cpuStatuses", shs, SLOT(onCPUStatuses(QVariantList)));
                     });

    Model *m = new Model;
    QObject::connect(m, &Model::propertyChanged, function(QByteArray propertyName) {
                         qDebug().nospace() << "Property " << propertyName << " changed to " << m->property(propertyName);
                     });
    m->setProperty("weight", 5);
    qDebug() << "Weight is:" << m->property("weight");

    messageBus->connect(QUrl("ws://localhost:8088"));

    return a.exec();
}
