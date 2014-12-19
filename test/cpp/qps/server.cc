/*
 *
 * Copyright 2014, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <thread>

#include <google/gflags.h>
#include <grpc/support/alloc.h>
#include <grpc/support/host_port.h>
#include <grpc++/config.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/status.h>
#include "test/cpp/interop/test.pb.h"

#include <grpc/grpc.h>
#include <grpc/support/log.h>

DEFINE_bool(enable_ssl, false, "Whether to use ssl/tls.");
DEFINE_int32(port, 0, "Server port.");

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::testing::Payload;
using grpc::testing::PayloadType;
using grpc::testing::SimpleRequest;
using grpc::testing::SimpleResponse;
using grpc::testing::TestService;
using grpc::Status;

bool SetPayload(PayloadType type, int size, Payload* payload) {
  PayloadType response_type = type;
  // TODO(yangg): Support UNCOMPRESSABLE payload.
  if (type != PayloadType::COMPRESSABLE) {
    return false;
  }
  payload->set_type(response_type);
  std::unique_ptr<char[]> body(new char[size]());
  payload->set_body(body.get(), size);
  return true;
}

class TestServiceImpl : public TestService::Service {
 public:
  Status UnaryCall(ServerContext* context, const SimpleRequest* request,
                   SimpleResponse* response) {
    if (request->has_response_size() && request->response_size() > 0) {
      if (!SetPayload(request->response_type(), request->response_size(),
                      response->mutable_payload())) {
        return Status(grpc::StatusCode::INTERNAL, "Error creating payload.");
      }
    }
    return Status::OK;
  }
};

void RunServer() {
  char* server_address = NULL;
  gpr_join_host_port(&server_address, "::", FLAGS_port);

  TestServiceImpl service;

  SimpleRequest request;
  SimpleResponse response;

  ServerBuilder builder;
  builder.AddPort(server_address);
  builder.RegisterService(service.service());
  std::unique_ptr<Server> server(builder.BuildAndStart());
  gpr_log(GPR_INFO, "Server listening on %s\n", server_address);
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  gpr_free(server_address);
}

int main(int argc, char** argv) {
  grpc_init();
  google::ParseCommandLineFlags(&argc, &argv, true);

  GPR_ASSERT(FLAGS_port != 0);
  GPR_ASSERT(!FLAGS_enable_ssl);
  RunServer();

  grpc_shutdown();
  return 0;
}