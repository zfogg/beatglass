#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <future>
#include <mutex>
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
