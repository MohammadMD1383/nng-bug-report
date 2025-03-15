#include <iostream>
#include <thread>
#include <vector>
#include "nng++.h"

using namespace std;
using namespace NNG;

void client() {
	const auto client = ReqRep::Client::New();
	if (!client) {
		cerr << "Failed to create client: " << client.err() << endl;
		return;
	}

	if (const auto r = client.get().dial("ipc://x"); !r) {
		cerr << "Failed to dial server: " << r.err() << endl;
		return;
	}

	vector<jthread> threads;

	for (int t = 0; t < 50; t++) {
		threads.emplace_back([&client = client.get()] {
			for (int i = 0; i < 1000; ++i) {
				srand(time(nullptr));
				const auto message = to_string(rand());
				const auto response = client.requestInParallel(message);
				if (!response) {
					cerr << "Failed sending request/getting response: " << response.err() << endl;
					continue;
				}

				if (response.get() != message) {
					cerr << "got bad response; expected: " << message << " actual: " << response.get() << endl;
				} else {
					cout << "ok" << endl;
				}
			}
		});
	}

	for (auto& thread: threads) {
		thread.join();
	}
}

void server() {
	const auto server = ReqRep::Server::New();
	if (!server) {
		cerr << "Failed to create server: " << server.err() << endl;
		return;
	}

	if (const auto r = server.get().listen("ipc://x"); !r) {
		cerr << "Failed to listen: " << r.err() << endl;
		return;
	}

	while (true) {
		const auto request = server.get().receiveInParallel();
		if (!request) {
			cerr << "Failed to receive request: " << request.err() << endl;
			continue;
		}

		if (const auto r = request.get().reply(request.get().message()); !r) {
			cerr << "Failed to send response: " << r.err() << endl;
		}
	}
}

int main(const int argc, const char* const argv[]) {
	if (argc != 2) {
	error:
		cout << "this program requires 1 argument: either `server` or `client`" << endl;
		return -1;
	}

	if (argv[1] == "client"sv) {
		client();
		return 0;
	}

	if (argv[1] == "server"sv) {
		server();
		return 0;
	}

	goto error;
}
