#pragma once
#include <atomic>

namespace Canceler {

enum class State : uint32_t {
	Generic = 0x00000000,
	Exchausted = 0x00000001,
	Abort = 0x00000002,
	Paused = 0x00000004,
};

class Canceler {
public:
	Canceler() = default;
	~Canceler() = default;

	void cancel(const State state = State::Generic) noexcept;
	void reset() noexcept;
	bool isCanceled() const noexcept;
	uint32_t getState() const noexcept;

private:
	std::atomic<uint32_t> m_state{0};
};

} // namespace Canceler
