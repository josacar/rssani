#ifndef GRPC_SERVER_H
#define GRPC_SERVER_H

#include <atomic>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "rssani.grpc.pb.h"
#include "rssani_lite.h"

/**
 * @brief Thread that runs the gRPC server.
 */
class GrpcServer : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs the gRPC server.
     * @param rss Pointer to the core application instance.
     * @param parent Parent QObject.
     */
    explicit GrpcServer(rssani_lite *rss, QObject *parent = nullptr);

    /**
     * @brief Destructor. Shuts down the gRPC server.
     */
    ~GrpcServer() override;

    /**
     * @brief Starts the gRPC server.
     */
    void start();

    /**
     * @brief Stops the gRPC server.
     */
    void stop();

private:
    rssani_lite *rss;
    std::unique_ptr<grpc::Server> server;
    std::atomic<bool> running{false};
};

/**
 * @brief gRPC service implementation.
 */
class RssaniServiceImpl final : public rssani::RssaniService::Service {
public:
    explicit RssaniServiceImpl(rssani_lite *rss);

    // Read-only methods
    grpc::Status VerUltimo(grpc::ServerContext *context,
                           const rssani::EmptyRequest *request,
                           rssani::StringResponse *response) override;

    grpc::Status VerTimer(grpc::ServerContext *context,
                          const rssani::EmptyRequest *request,
                          rssani::IntResponse *response) override;

    grpc::Status VerLog(grpc::ServerContext *context,
                        const rssani::LogRequest *request,
                        rssani::LogResponse *response) override;

    grpc::Status ListaExpresiones(grpc::ServerContext *context,
                                  const rssani::EmptyRequest *request,
                                  rssani::RegexpListResponse *response) override;

    grpc::Status ListaAuths(grpc::ServerContext *context,
                            const rssani::EmptyRequest *request,
                            rssani::AuthListResponse *response) override;

    grpc::Status VerOpciones(grpc::ServerContext *context,
                             const rssani::EmptyRequest *request,
                             rssani::OpcionesResponse *response) override;

    // Regexp CRUD
    grpc::Status AnadirRegexp(grpc::ServerContext *context,
                              const rssani::AnadirRegexpRequest *request,
                              rssani::BoolResponse *response) override;

    grpc::Status EditarRegexp(grpc::ServerContext *context,
                              const rssani::EditarRegexpRequest *request,
                              rssani::BoolResponse *response) override;

    grpc::Status EditarRegexpI(grpc::ServerContext *context,
                               const rssani::EditarRegexpIRequest *request,
                               rssani::BoolResponse *response) override;

    grpc::Status ActivarRegexp(grpc::ServerContext *context,
                               const rssani::ActivarRegexpRequest *request,
                               rssani::BoolResponse *response) override;

    grpc::Status MoverRegexp(grpc::ServerContext *context,
                             const rssani::MoverRegexpRequest *request,
                             rssani::BoolResponse *response) override;

    grpc::Status BorrarRegexpI(grpc::ServerContext *context,
                               const rssani::BorrarRegexpIRequest *request,
                               rssani::BoolResponse *response) override;

    grpc::Status BorrarRegexpS(grpc::ServerContext *context,
                               const rssani::BorrarRegexpSRequest *request,
                               rssani::BoolResponse *response) override;

    // Auth CRUD
    grpc::Status AnadirAuth(grpc::ServerContext *context,
                            const rssani::AnadirAuthRequest *request,
                            rssani::BoolResponse *response) override;

    grpc::Status BorrarAuth(grpc::ServerContext *context,
                            const rssani::BorrarAuthRequest *request,
                            rssani::BoolResponse *response) override;

    // Options and config
    grpc::Status PonerCredenciales(grpc::ServerContext *context,
                                   const rssani::PonerCredencialesRequest *request,
                                   rssani::BoolResponse *response) override;

    grpc::Status PonerOpciones(grpc::ServerContext *context,
                               const rssani::PonerOpcionesRequest *request,
                               rssani::BoolResponse *response) override;

    grpc::Status CambiaTimer(grpc::ServerContext *context,
                             const rssani::CambiaTimerRequest *request,
                             rssani::BoolResponse *response) override;

    grpc::Status Guardar(grpc::ServerContext *context,
                         const rssani::EmptyRequest *request,
                         rssani::BoolResponse *response) override;

    grpc::Status Shutdown(grpc::ServerContext *context,
                          const rssani::EmptyRequest *request,
                          rssani::BoolResponse *response) override;

private:
    rssani_lite *rss;
};

#endif
