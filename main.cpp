#include <iostream>
#include <thread>
#include <vector>
#include "nng++.h"

using namespace std;
using namespace NNG;

void client() {
	const auto client = ReqRep::Client::New().takeOwnership();
	client.dial("ipc://x").ignore();

	vector<jthread> threads;

	for (int t = 0; t < 50; t++) {
		threads.emplace_back([&client] {
			for (int i = 0; i < 1000; ++i) {
				srand(time(nullptr));
				const auto message = to_string(rand());
				const auto response = client.requestInParallel(message).takeOwnership();
				if (response != message) {
					cerr << "got bad response; expected: " << message << " actual: " << response << endl;
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

[[noreturn]]
void server() {
	const auto server = ReqRep::Server::New().takeOwnership();
	server.listen("ipc://x").ignore();

	while (true) {
		const auto request = server.receiveInParallel().takeOwnership();
		request.reply(request.message()).ignore();
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
