#include "xmlrpc.h"
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

// --- Métodos XML-RPC como clases xmlrpc_c::method2 ---

class VerUltimo : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    VerUltimo(rssani_lite *r) : rss(r) {
        _signature = "s:";
        _help = "Devuelve el instante del ultimo get";
    }
    void execute(xmlrpc_c::paramList const&, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        *retvalP = xmlrpc_c::value_string(rss->verUltimo());
    }
};

class VerTimer : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    VerTimer(rssani_lite *r) : rss(r) {
        _signature = "i:";
        _help = "Devuelve el timer restante en ms";
    }
    void execute(xmlrpc_c::paramList const&, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        *retvalP = xmlrpc_c::value_int(rss->verTimer());
    }
};

class VerLog : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    VerLog(rssani_lite *r) : rss(r) {
        _signature = "A:ii";
        _help = "Devuelve el log";
    }
    void execute(xmlrpc_c::paramList const& params, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        int ini = params.getInt(0);
        int fin = params.getInt(1);
        QStringList log = rss->verLog();
        if (fin == 0) fin = log.size();
        std::vector<xmlrpc_c::value> valores;
        for (int i = log.size() - ini - 1; i >= std::max(log.size() - fin, qsizetype(0)); i--)
            valores.push_back(xmlrpc_c::value_string(log.at(i).toStdString()));
        *retvalP = xmlrpc_c::value_array(valores);
    }
};

class ListaExpresiones : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    ListaExpresiones(rssani_lite *r) : rss(r) {
        _signature = "A:";
        _help = "Devuelve las expresiones regulares";
    }
    void execute(xmlrpc_c::paramList const&, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        QList<regexp*> *log = rss->listaRegexp();
        std::vector<xmlrpc_c::value> valores;
        for (const auto &item : *log) {
            std::map<std::string, xmlrpc_c::value> m;
            m["nombre"]      = xmlrpc_c::value_string(item->nombre.toStdString());
            m["vencimiento"] = xmlrpc_c::value_string(item->vencimiento.toStdString());
            m["mail"]        = xmlrpc_c::value_boolean(item->mail);
            m["tracker"]     = xmlrpc_c::value_string(item->tracker.toStdString());
            m["dias"]        = xmlrpc_c::value_int(item->diasDescarga);
            m["activa"]      = xmlrpc_c::value_boolean(item->activa);
            valores.push_back(xmlrpc_c::value_struct(m));
        }
        *retvalP = xmlrpc_c::value_array(valores);
    }
};

class EditarRegexp : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    EditarRegexp(rssani_lite *r) : rss(r) {
        _signature = "b:ss";
        _help = "Edita una regexp de la lista";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        *retvalP = xmlrpc_c::value_boolean(
            rss->editarRegexp(p.getString(0), p.getString(1)));
    }
};

class EditarRegexpI : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    EditarRegexpI(rssani_lite *r) : rss(r) {
        _signature = "b:is";
        _help = "Edita una regexp de la lista";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        *retvalP = xmlrpc_c::value_boolean(
            rss->editarRegexp(p.getInt(0), p.getString(1)));
    }
};

class ActivarRegexp : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    ActivarRegexp(rssani_lite *r) : rss(r) {
        _signature = "b:i";
        _help = "(Des)Activa una regexp de la lista";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        *retvalP = xmlrpc_c::value_boolean(rss->activarRegexp(p.getInt(0)));
    }
};

class AnadirRegexp : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    AnadirRegexp(rssani_lite *r) : rss(r) {
        _signature = "b:ssbsi";
        _help = "Añade una regexp a la lista";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        rss->anadirRegexp(p.getString(0), p.getString(1),
                          p.getBoolean(2), p.getString(3), p.getInt(4));
        *retvalP = xmlrpc_c::value_boolean(true);
    }
};

class MoverRegexp : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    MoverRegexp(rssani_lite *r) : rss(r) {
        _signature = "b:ii";
        _help = "Mueve una regexp de la lista";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        rss->moverRegexp(p.getInt(0), p.getInt(1));
        *retvalP = xmlrpc_c::value_boolean(true);
    }
};

class BorrarRegexpI : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    BorrarRegexpI(rssani_lite *r) : rss(r) {
        _signature = "b:i";
        _help = "Borra la expresion 'i' de la lista";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        rss->borrarRegexp(p.getInt(0));
        *retvalP = xmlrpc_c::value_boolean(true);
    }
};

class BorrarRegexpS : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    BorrarRegexpS(rssani_lite *r) : rss(r) {
        _signature = "b:s";
        _help = "Borra la expresion de la lista";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        rss->borrarRegexp(p.getString(0));
        *retvalP = xmlrpc_c::value_boolean(true);
    }
};

class AnadirAuth : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    AnadirAuth(rssani_lite *r) : rss(r) {
        _signature = "b:ssss";
        _help = "Añade un tracker con su auth";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        rss->anadirAuth(p.getString(0), p.getString(1),
                        p.getString(2), p.getString(3));
        *retvalP = xmlrpc_c::value_boolean(true);
    }
};

class BorrarAuth : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    BorrarAuth(rssani_lite *r) : rss(r) {
        _signature = "b:s";
        _help = "Borra el auth del tracker";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        rss->borrarAuth(p.getString(0));
        *retvalP = xmlrpc_c::value_boolean(true);
    }
};

class ListaAuths : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    ListaAuths(rssani_lite *r) : rss(r) {
        _signature = "A:";
        _help = "Lista los auth de los trackers";
    }
    void execute(xmlrpc_c::paramList const&, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        QList<auth> *auths = rss->listaAuths();
        std::vector<xmlrpc_c::value> valores;
        for (const auto &item : *auths) {
            std::map<std::string, xmlrpc_c::value> m;
            m["tracker"] = xmlrpc_c::value_string(item.tracker.toStdString());
            m["uid"]     = xmlrpc_c::value_string(item.uid.toStdString());
            m["pass"]    = xmlrpc_c::value_string(item.pass.toStdString());
            m["passkey"] = xmlrpc_c::value_string(item.passkey.toStdString());
            valores.push_back(xmlrpc_c::value_struct(m));
        }
        *retvalP = xmlrpc_c::value_array(valores);
    }
};

class PonerCredenciales : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    PonerCredenciales(rssani_lite *r) : rss(r) {
        _signature = "b:ss";
        _help = "Establece los credenciales RPC para rssani";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        rss->setRpcUser(p.getString(0));
        rss->setRpcPass(p.getString(1));
        qDebug() << "USER:" << rss->getRpcUser() << "PASS:" << rss->getRpcPass();
        *retvalP = xmlrpc_c::value_boolean(true);
    }
};

class VerOpciones : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    VerOpciones(rssani_lite *r) : rss(r) {
        _signature = "S:";
        _help = "Muestra las opciones generales";
    }
    void execute(xmlrpc_c::paramList const&, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        Values *opciones = rss->getValues();
        std::map<std::string, xmlrpc_c::value> m;
        m["fromMail"] = xmlrpc_c::value_string(opciones->FromMail().toStdString());
        m["toMail"]   = xmlrpc_c::value_string(opciones->ToMail().toStdString());
        m["path"]     = xmlrpc_c::value_string(opciones->Ruta().toStdString());
        *retvalP = xmlrpc_c::value_struct(m);
    }
};

class PonerOpciones : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    PonerOpciones(rssani_lite *r) : rss(r) {
        _signature = "b:sss";
        _help = "Pone las opciones generales";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        Values *opciones = rss->getValues();
        opciones->setFromMail(QString::fromStdString(p.getString(0)));
        opciones->setToMail(QString::fromStdString(p.getString(1)));
        opciones->setRuta(QString::fromStdString(p.getString(2)));
        *retvalP = xmlrpc_c::value_boolean(true);
    }
};

class CambiaTimer : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    CambiaTimer(rssani_lite *r) : rss(r) {
        _signature = "b:i";
        _help = "Cambia el timer de descarga del RSS";
    }
    void execute(xmlrpc_c::paramList const& p, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        rss->cambiaTimer(p.getInt(0));
        *retvalP = xmlrpc_c::value_boolean(true);
    }
};

class Guardar : public xmlrpc_c::method2 {
    rssani_lite *rss;
public:
    Guardar(rssani_lite *r) : rss(r) {
        _signature = "b:";
        _help = "Guarda la configuración";
    }
    void execute(xmlrpc_c::paramList const&, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        rss->guardar();
        *retvalP = xmlrpc_c::value_boolean(true);
    }
};

class Shutdown : public xmlrpc_c::method2 {
    rssani_lite *rss;
    xmlrpc_c::serverAbyss *server;
public:
    Shutdown(rssani_lite *r) : rss(r), server(nullptr) {
        _signature = "b:";
        _help = "Cierra RSSANI";
    }
    void setServer(xmlrpc_c::serverAbyss *s) { server = s; }
    void execute(xmlrpc_c::paramList const&, const xmlrpc_c::callInfo *,
                 xmlrpc_c::value * const retvalP) override {
        qDebug() << "Metodos ha recibido la señal de apagado";
        *retvalP = xmlrpc_c::value_boolean(true);
        qDebug() << "Terminando..";
        rss->salir();
        if (server) server->terminate();
        qDebug() << "Retornando..";
    }
};

// --- Hilo XML-RPC ---

rssxmlrpc::rssxmlrpc(rssani_lite *rss) : QThread(), rss(rss) {}

rssxmlrpc::~rssxmlrpc() {
    if (auto *s = server.load()) s->terminate();
    wait();
}

void rssxmlrpc::run() {
    xmlrpc_c::registry registry;

    auto shutdownMethod = new Shutdown(rss);

    registry.addMethod("rssani.verUltimo",        xmlrpc_c::methodPtr(new VerUltimo(rss)));
    registry.addMethod("rssani.verTimer",          xmlrpc_c::methodPtr(new VerTimer(rss)));
    registry.addMethod("rssani.verLog",            xmlrpc_c::methodPtr(new VerLog(rss)));
    registry.addMethod("rssani.listaExpresiones",  xmlrpc_c::methodPtr(new ListaExpresiones(rss)));
    registry.addMethod("rssani.editarRegexp",      xmlrpc_c::methodPtr(new EditarRegexp(rss)));
    registry.addMethod("rssani.editarRegexpI",     xmlrpc_c::methodPtr(new EditarRegexpI(rss)));
    registry.addMethod("rssani.activarRegexp",     xmlrpc_c::methodPtr(new ActivarRegexp(rss)));
    registry.addMethod("rssani.anadirRegexp",      xmlrpc_c::methodPtr(new AnadirRegexp(rss)));
    registry.addMethod("rssani.moverRegexp",       xmlrpc_c::methodPtr(new MoverRegexp(rss)));
    registry.addMethod("rssani.borrarRegexpI",     xmlrpc_c::methodPtr(new BorrarRegexpI(rss)));
    registry.addMethod("rssani.borrarRegexpS",     xmlrpc_c::methodPtr(new BorrarRegexpS(rss)));
    registry.addMethod("rssani.anadirAuth",        xmlrpc_c::methodPtr(new AnadirAuth(rss)));
    registry.addMethod("rssani.borrarAuth",        xmlrpc_c::methodPtr(new BorrarAuth(rss)));
    registry.addMethod("rssani.listaAuths",        xmlrpc_c::methodPtr(new ListaAuths(rss)));
    registry.addMethod("rssani.ponerCredenciales", xmlrpc_c::methodPtr(new PonerCredenciales(rss)));
    registry.addMethod("rssani.verOpciones",       xmlrpc_c::methodPtr(new VerOpciones(rss)));
    registry.addMethod("rssani.ponerOpciones",     xmlrpc_c::methodPtr(new PonerOpciones(rss)));
    registry.addMethod("rssani.cambiaTimer",       xmlrpc_c::methodPtr(new CambiaTimer(rss)));
    registry.addMethod("rssani.guardar",           xmlrpc_c::methodPtr(new Guardar(rss)));
    registry.addMethod("rssani.shutdown",          xmlrpc_c::methodPtr(shutdownMethod));

    xmlrpc_c::serverAbyss srv(
        xmlrpc_c::serverAbyss::constrOpt()
            .registryP(&registry)
            .portNumber(8080)
            .serverOwnsSignals(false)
    );

    shutdownMethod->setServer(&srv);

    server.store(&srv);
    srv.run();
}
