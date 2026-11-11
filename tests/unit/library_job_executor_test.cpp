#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#include "beatglass/library/detail/job_executor.hpp"
using namespace std::chrono_literals;
using beatglass::library_detail::JobExecutor;

TEST_CASE("library executor bounds waiting work and orders priorities") {
  JobExecutor executor(3);
  std::promise<void> started, release;
  auto gate = release.get_future().share();
  REQUIRE(executor.submit(0, [&](std::stop_token) { started.set_value(); gate.wait(); }));
  REQUIRE(started.get_future().wait_for(5s) == std::future_status::ready);
  std::mutex mutex;
  std::vector<int> order;
  auto add = [&](int n) { return [&, n](std::stop_token) { std::lock_guard lock(mutex); order.push_back(n); }; };
  std::promise<void> done;
  REQUIRE(executor.submit(1, [&](std::stop_token stop) { add(1)(stop); done.set_value(); }));
  REQUIRE(executor.submit(2, add(2)));
  REQUIRE(executor.submit(2, add(3)));
  REQUIRE_FALSE(executor.submit(3, add(4)));
  release.set_value();
  REQUIRE(done.get_future().wait_for(5s) == std::future_status::ready);
  executor.shutdown();
  REQUIRE(order == std::vector<int>{2, 3, 1});
  REQUIRE_FALSE(executor.submit(0, add(5)));
}

TEST_CASE("zero-capacity executor rejects all submissions") {
  JobExecutor executor(0);
  bool ran = false;
  REQUIRE_FALSE(executor.submit(0, [&](std::stop_token) { ran = true; }));
  REQUIRE_FALSE(executor.submit_tracked(0, [&](std::stop_token, const auto&) {
    ran = true; return beatglass::library_detail::TrackedState::completed;
  }));
  executor.shutdown();
  REQUIRE_FALSE(ran);
}
TEST_CASE("executor rejects null work without creating a tracked job") {
  JobExecutor executor(1);
  REQUIRE_FALSE(executor.submit(0, {}));
  REQUIRE_FALSE(executor.submit_tracked(0, {}));
  executor.shutdown();
}

TEST_CASE("library executor cancels active work and discards waiting jobs") {
  JobExecutor executor(1);
  std::promise<void> started;
  bool observed = false;
  bool queued_ran = false;
  REQUIRE(executor.submit(0, [&](std::stop_token stop) {
    started.set_value();
    while (!stop.stop_requested()) std::this_thread::yield();
    observed = true;
  }));
  REQUIRE(started.get_future().wait_for(5s) == std::future_status::ready);
  REQUIRE(executor.submit(0, [&](std::stop_token) { queued_ran = true; }));
  executor.shutdown();
  REQUIRE(observed);
  REQUIRE_FALSE(queued_ran);
}

TEST_CASE("tracked jobs expose individual cancellation and monotonic progress") {
  using beatglass::library_detail::TrackedState;
  JobExecutor executor(2);
  std::promise<void> started, resume; auto gate = resume.get_future().share();
  auto active = executor.submit_tracked(0, [&](std::stop_token stop, const auto& progress) {
    progress(12); progress(4); started.set_value(); gate.wait();
    return stop.stop_requested() ? TrackedState::cancelled : TrackedState::completed;
  });
  REQUIRE(active); REQUIRE(started.get_future().wait_for(5s) == std::future_status::ready);
  REQUIRE(active->state() == TrackedState::running); REQUIRE(active->frames() == 12);
  bool ran = false;
  auto queued = executor.submit_tracked(0, [&](std::stop_token, const auto&) { ran = true; return TrackedState::completed; });
  REQUIRE(queued); queued->cancel(); REQUIRE(queued->state() == TrackedState::cancelled);
  active->cancel(); resume.set_value(); executor.shutdown();
  REQUIRE(active->state() == TrackedState::cancelled); REQUIRE_FALSE(ran);
}
TEST_CASE("tracked job completion wins over a late cancellation") {
  using beatglass::library_detail::TrackedState;
  JobExecutor executor(1);
  std::promise<void> started, resume; auto gate = resume.get_future().share();
  auto handle = executor.submit_tracked(0, [&](std::stop_token, const auto&) {
    started.set_value(); gate.wait(); return TrackedState::completed;
  });
  REQUIRE(handle); REQUIRE(started.get_future().wait_for(5s) == std::future_status::ready);
  handle->cancel(); resume.set_value(); executor.shutdown();
  REQUIRE(handle->state() == TrackedState::completed);
}

TEST_CASE("tracked failures and discarded jobs remain observable after shutdown") {
  using beatglass::library_detail::TrackedState;
  std::shared_ptr<beatglass::library_detail::JobHandle> failed, discarded;
  {
    JobExecutor executor(1);
    std::promise<void> started, release; auto gate = release.get_future().share();
    failed = executor.submit_tracked(0, [&](std::stop_token, const auto& progress) -> TrackedState {
      progress(7); started.set_value(); gate.wait(); throw std::runtime_error("failed");
    });
    REQUIRE(failed); REQUIRE(started.get_future().wait_for(5s) == std::future_status::ready);
    discarded = executor.submit_tracked(0, [](std::stop_token, const auto&) { return TrackedState::completed; });
    REQUIRE(discarded);
    auto shutdown = std::async(std::launch::async, [&] { executor.shutdown(); });
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (discarded->state() == TrackedState::queued && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    const auto queued_state = discarded->state();
    release.set_value();
    REQUIRE(queued_state == TrackedState::cancelled);
    REQUIRE(shutdown.wait_for(5s) == std::future_status::ready);
    shutdown.get();
  }
  REQUIRE(failed->state() == TrackedState::failed);
  REQUIRE(failed->frames() == 7);
  REQUIRE(discarded->state() == TrackedState::cancelled);
}

TEST_CASE("shutdown destroys discarded work outside the queue lock") {
  struct ReenterOnDestruction {
    JobExecutor* executor;
    std::promise<bool>* result;
    ~ReenterOnDestruction() {
      result->set_value(executor->submit(0, [](std::stop_token) {}));
    }
  };
  JobExecutor executor(1);
  std::promise<void> started;
  std::promise<bool> reentered;
  REQUIRE(executor.submit(0, [&](std::stop_token stop) {
    started.set_value();
    while (!stop.stop_requested()) std::this_thread::yield();
  }));
  REQUIRE(started.get_future().wait_for(5s) == std::future_status::ready);
  auto capture = std::make_shared<ReenterOnDestruction>(&executor, &reentered);
  REQUIRE(executor.submit(0, [capture](std::stop_token) {}));
  capture.reset();
  executor.shutdown();
  auto result = reentered.get_future();
  REQUIRE(result.wait_for(5s) == std::future_status::ready);
  REQUIRE_FALSE(result.get());
}

TEST_CASE("terminal tracked jobs reject late progress and cancellation") {
  using beatglass::library_detail::TrackedState;
  JobExecutor executor(1);
  std::function<void(std::uint64_t)> retained;
  std::stop_token token;
  auto handle = executor.submit_tracked(0, [&](std::stop_token stop, const auto& progress) {
    token = stop; retained = progress; progress(3); return TrackedState::completed;
  });
  REQUIRE(handle);
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while (handle->state() != TrackedState::completed && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
  REQUIRE(handle->state() == TrackedState::completed);
  retained(100);
  REQUIRE(handle->frames() == 3);
  bool called = false;
  std::stop_callback callback(token, [&] { called = true; });
  handle->cancel();
  REQUIRE_FALSE(called);
  REQUIRE_FALSE(token.stop_requested());
  executor.shutdown();
}

TEST_CASE("tracked cancellation runs stop callbacks outside the state lock") {
  using beatglass::library_detail::TrackedState;
  JobExecutor executor(1);
  std::promise<void> armed, complete;
  std::promise<std::shared_ptr<beatglass::library_detail::JobHandle>> published;
  auto future = published.get_future().share();
  auto handle = executor.submit_tracked(0, [&](std::stop_token stop, const auto&) {
    auto self = future.get();
    std::stop_callback callback(stop, [&] { (void)self->state(); complete.set_value(); });
    armed.set_value();
    while (!stop.stop_requested()) std::this_thread::yield();
    return TrackedState::cancelled;
  });
  REQUIRE(handle);
  published.set_value(handle);
  // Full qualification runs this binary alongside hundreds of filesystem and
  // native-contract tests; the callback itself must remain bounded, but the
  // harness should not turn transient Windows scheduler contention into a
  // false cancellation failure.
  REQUIRE(armed.get_future().wait_for(30s) == std::future_status::ready);
  handle->cancel();
  REQUIRE(complete.get_future().wait_for(30s) == std::future_status::ready);
  executor.shutdown();
  REQUIRE(handle->state() == TrackedState::cancelled);
}

TEST_CASE("shutdown signals active work before destroying queued closures") {
  struct WaitForStop {
    std::shared_future<void> observed;
    std::promise<bool>* result;
    ~WaitForStop() { result->set_value(observed.wait_for(5s) == std::future_status::ready); }
  };
  JobExecutor executor(1);
  std::promise<void> started, observed;
  std::promise<bool> destructor_saw_stop;
  REQUIRE(executor.submit(0, [&](std::stop_token stop) {
    started.set_value();
    while (!stop.stop_requested()) std::this_thread::yield();
    observed.set_value();
  }));
  REQUIRE(started.get_future().wait_for(5s) == std::future_status::ready);
  auto capture = std::make_shared<WaitForStop>(observed.get_future().share(), &destructor_saw_stop);
  REQUIRE(executor.submit(0, [capture](std::stop_token) {}));
  capture.reset();
  executor.shutdown();
  REQUIRE(destructor_saw_stop.get_future().get());
}
