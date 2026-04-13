#include "grpc_server.h"
#include <QDebug>
#include <thread>

// --- gRPC Service Implementation ---

RssaniServiceImpl::RssaniServiceImpl(rssani_lite *rss) : rss(rss) {}

// Read-only methods

grpc::Status RssaniServiceImpl::VerUltimo(grpc::ServerContext *,
                                           const rssani::EmptyRequest *,
                                           rssani::StringResponse *response) {
    response->set_value(rss->verUltimo());
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::VerTimer(grpc::ServerContext *,
                                          const rssani::EmptyRequest *,
                                          rssani::IntResponse *response) {
    response->set_value(rss->verTimer());
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::VerLog(grpc::ServerContext *,
                                        const rssani::LogRequest *request,
                                        rssani::LogResponse *response) {
    int ini = request->ini();
    int fin = request->fin();
    QStringList log = rss->verLog();
    if (fin == 0) fin = log.size();
    for (int i = log.size() - ini - 1; i >= std::max(log.size() - fin, qsizetype(0)); i--)
        response->add_lines(log.at(i).toStdString());
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::ListaExpresiones(grpc::ServerContext *,
                                                  const rssani::EmptyRequest *,
                                                  rssani::RegexpListResponse *response) {
    QList<regexp*> *log = rss->listaRegexp();
    for (const auto &item : *log) {
        auto *entry = response->add_entries();
        entry->set_nombre(item->nombre.toStdString());
        entry->set_vencimiento(item->vencimiento.toStdString());
        entry->set_mail(item->mail);
        entry->set_tracker(item->tracker.toStdString());
        entry->set_dias(item->diasDescarga);
        entry->set_activa(item->activa);
    }
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::ListaAuths(grpc::ServerContext *,
                                            const rssani::EmptyRequest *,
                                            rssani::AuthListResponse *response) {
    QList<auth> *auths = rss->listaAuths();
    for (const auto &item : *auths) {
        auto *entry = response->add_entries();
        entry->set_tracker(item.tracker.toStdString());
        entry->set_uid(item.uid.toStdString());
        entry->set_pass(item.pass.toStdString());
        entry->set_passkey(item.passkey.toStdString());
    }
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::VerOpciones(grpc::ServerContext *,
                                             const rssani::EmptyRequest *,
                                             rssani::OpcionesResponse *response) {
    Values *opciones = rss->getValues();
    response->set_frommail(opciones->FromMail().toStdString());
    response->set_tomail(opciones->ToMail().toStdString());
    response->set_path(opciones->Ruta().toStdString());
    return grpc::Status::OK;
}

// Regexp CRUD

grpc::Status RssaniServiceImpl::AnadirRegexp(grpc::ServerContext *,
                                              const rssani::AnadirRegexpRequest *request,
                                              rssani::BoolResponse *response) {
    rss->anadirRegexp(request->nombre(), request->fecha(),
                      request->mail(), request->tracker(), request->dias());
    response->set_value(true);
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::EditarRegexp(grpc::ServerContext *,
                                              const rssani::EditarRegexpRequest *request,
                                              rssani::BoolResponse *response) {
    response->set_value(rss->editarRegexp(request->regexporig(), request->regexpdest()));
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::EditarRegexpI(grpc::ServerContext *,
                                               const rssani::EditarRegexpIRequest *request,
                                               rssani::BoolResponse *response) {
    response->set_value(rss->editarRegexp(request->regexporig(), request->regexpdest()));
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::ActivarRegexp(grpc::ServerContext *,
                                               const rssani::ActivarRegexpRequest *request,
                                               rssani::BoolResponse *response) {
    response->set_value(rss->activarRegexp(request->regexporig()));
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::MoverRegexp(grpc::ServerContext *,
                                             const rssani::MoverRegexpRequest *request,
                                             rssani::BoolResponse *response) {
    rss->moverRegexp(request->from_position(), request->to());
    response->set_value(true);
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::BorrarRegexpI(grpc::ServerContext *,
                                               const rssani::BorrarRegexpIRequest *request,
                                               rssani::BoolResponse *response) {
    rss->borrarRegexp(request->pos());
    response->set_value(true);
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::BorrarRegexpS(grpc::ServerContext *,
                                               const rssani::BorrarRegexpSRequest *request,
                                               rssani::BoolResponse *response) {
    rss->borrarRegexp(request->nombre());
    response->set_value(true);
    return grpc::Status::OK;
}

// Auth CRUD

grpc::Status RssaniServiceImpl::AnadirAuth(grpc::ServerContext *,
                                            const rssani::AnadirAuthRequest *request,
                                            rssani::BoolResponse *response) {
    rss->anadirAuth(request->tracker(), request->uid(),
                    request->password(), request->passkey());
    response->set_value(true);
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::BorrarAuth(grpc::ServerContext *,
                                            const rssani::BorrarAuthRequest *request,
                                            rssani::BoolResponse *response) {
    rss->borrarAuth(request->tracker());
    response->set_value(true);
    return grpc::Status::OK;
}

// Options and config

grpc::Status RssaniServiceImpl::PonerCredenciales(grpc::ServerContext *,
                                                   const rssani::PonerCredencialesRequest *request,
                                                   rssani::BoolResponse *response) {
    rss->setRpcUser(request->user());
    rss->setRpcPass(request->password());
    qDebug() << "USER:" << rss->getRpcUser() << "PASS:" << rss->getRpcPass();
    response->set_value(true);
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::PonerOpciones(grpc::ServerContext *,
                                               const rssani::PonerOpcionesRequest *request,
                                               rssani::BoolResponse *response) {
    Values *opciones = rss->getValues();
    opciones->setFromMail(QString::fromStdString(request->frommail()));
    opciones->setToMail(QString::fromStdString(request->tomail()));
    opciones->setRuta(QString::fromStdString(request->path()));
    response->set_value(true);
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::CambiaTimer(grpc::ServerContext *,
                                             const rssani::CambiaTimerRequest *request,
                                             rssani::BoolResponse *response) {
    rss->cambiaTimer(request->tiempo());
    response->set_value(true);
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::Guardar(grpc::ServerContext *,
                                         const rssani::EmptyRequest *,
                                         rssani::BoolResponse *response) {
    rss->guardar();
    response->set_value(true);
    return grpc::Status::OK;
}

grpc::Status RssaniServiceImpl::Shutdown(grpc::ServerContext *,
                                          const rssani::EmptyRequest *,
                                          rssani::BoolResponse *response) {
    qDebug() << "gRPC server received shutdown signal";
    response->set_value(true);
    qDebug() << "Shutting down...";
    rss->salir();
    return grpc::Status::OK;
}

// --- GrpcServer ---

GrpcServer::GrpcServer(rssani_lite *rss, QObject *parent)
    : QObject(parent), rss(rss) {}

GrpcServer::~GrpcServer() {
    stop();
}

void GrpcServer::start() {
    if (running.exchange(true)) return;

    std::thread([this]() {
        RssaniServiceImpl service(rss);

        grpc::ServerBuilder builder;
        builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
        builder.RegisterService(&service);

        server = builder.BuildAndStart();
        if (!server) {
            qCritical() << "Failed to start gRPC server";
            running.store(false);
            return;
        }

        qDebug() << "gRPC server listening on 0.0.0.0:50051";
        server->Wait();
        qDebug() << "gRPC server stopped";
        running.store(false);
    }).detach();
}

void GrpcServer::stop() {
    if (!running.exchange(false)) return;
    if (server) {
        server->Shutdown();
    }
}
