#include "base.hpp"
#include "position.hpp"
#include "engine.hpp"

enum EventType { kCommandEvent, kSearchResultEvent, kNoEventType };

struct Event {
  EventType type = kNoEventType;
  // TODO:
  //   std::variant<string, SearchResult> should be fine but the compiler on CI didn't accept some code.
  //   So, for now, we simply hold both two exclusive entries.
  string command;
  SearchResult search_result;
};

struct UCI {
  std::istream& istr;
  std::ostream& ostr;
  std::ostream& err_ostr;
  Queue<Event> queue; // single consumer and two producers

  // TODO: we can use std::future to make sure spawned threads are joined properly in the end
  std::future<void> command_listener_thread_future;
  std::future<void> engine_thread_future;

  Engine engine;
  Position position;

  UCI(std::istream& istr, std::ostream& ostr, std::ostream& err_ostr)
    : istr{istr}, ostr{ostr}, err_ostr{err_ostr} {

    // Setup callback for the engine
    // TODO: Make sure engine thread won't call this after UCI (thus Queue) is destructed
    engine.search_result_callback = [&](const SearchResult& search_result) {
      queue.put(Event({ .type = kSearchResultEvent, .search_result = search_result }));
    };
  }

  int mainLoop() {
    startCommandListenerThread();
    monitorEvent();
    return 0;
  }

  void startCommandListenerThread();
  void monitorEvent();
  void handleCommand(const string&);
  void handleSearchResult(const SearchResult&);

  //
  // I/O helpers
  //

  string readToken(std::istream& istream) {
    string res;
    istream >> res;
    return res;
  }

  template<class T>
  void print(const T& v) { ostr << v << std::endl; }

  template<class T>
  void printError(const T& v) { err_ostr << "ERROR: " << v << std::endl; }

  //
  // UCI commands
  //

  void uci_uci(std::istream&) {
    print("name toy-chess");
    print("author hiro18181");
    print("uciok");
  }

  void uci_debug(std::istream& command) {
    // debug [ on | off ]
    auto token = readToken(command);
    assert(token == "on" || token == "off");
    printError("Unsupported command");
  }

  void uci_isready(std::istream&) {
    print("readyok");
  }

  void uci_setoption(std::istream&) {
    // setoption name <id> [value <x>]
    printError("Unsupported command");
  }

  void uci_register(std::istream&) {
    printError("Unsupported command");
  }

  void uci_ucinewgame(std::istream&) {
    printError("Unsupported command");
  }

  void uci_position(std::istream&);
  void uci_go(std::istream&);

  void uci_stop(std::istream&) {
    engine.stop();
  }

  void uci_ponderhit(std::istream&) {
    printError("Unsupported command");
  }
};