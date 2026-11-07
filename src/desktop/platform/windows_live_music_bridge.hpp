#pragma once
#include "music_ui_handoff.hpp"
#include "windows_live_music.hpp"
#include <QObject>
#include <QTimer>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
Q_DECLARE_METATYPE(beatglass::windows::MusicUiUpdate)
namespace beatglass::windows {
/** Experimental selected-output composite bridge. Construct and control on the
 * GUI thread; a single owned worker drains LoopbackSession and runs DSP. It
 * coalesces at most one pending owned update and gates deliveries on the GUI
 * thread. This does not create controls, select a default endpoint, or cache.
 * The allocator outlives the bridge and worker. Essentia construction across
 * workers requires external serialization until its factory is qualified.
 */
class LiveMusicBridge final : public QObject {
  Q_OBJECT
public:
  /** Borrow a process-owned epoch allocator; parent lives on the GUI thread.
   * Optional startHook is a worker-only test seam and must not retain arguments. */
  explicit LiveMusicBridge(SourceEpochAllocator& epochs,QObject* parent=nullptr,
                           std::function<bool(LiveMusic&,const std::string&,std::shared_ptr<LoopbackStartAuthorization>)> startHook={});
  // Documented manually in the API reference due to Hawkmoth destructor parsing.
  ~LiveMusicBridge() override;
  /** Request one selected endpoint; false for empty ID or exhausted sessions.
   * Invalidates prior GUI state immediately; actual capture starts asynchronously.
   * A claimed native start can remain in flight until its native call returns. */
  bool start(const QString& endpoint);
  /** Invalidate prior state immediately and request worker stop. */
  void stop();
  /** GUI-thread update or terminal error clear; receivers must gate queued deliveries and label estimates as provisional windows. */
  Q_SIGNAL void musicUpdate(beatglass::windows::MusicUiUpdate update);
private:
  void run();
  void drain();
  bool advanceSession();
  SourceEpochAllocator& epochs_;
  std::function<bool(LiveMusic&,const std::string&,std::shared_ptr<LoopbackStartAuthorization>)> start_hook_;
  std::shared_ptr<LoopbackStartAuthorization> authorization_;
  std::mutex mutex_;
  std::condition_variable wake_;
  std::optional<MusicUiUpdate> pending_;
  std::string endpoint_;
  std::uint64_t session_{};
  bool requested_{}, quitting_{}, exhausted_{};
  MusicUiGate gate_;
  QTimer timer_;
  std::thread worker_;
};
}
