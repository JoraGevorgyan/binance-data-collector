#include "../Canceler.hpp"

namespace Canceler {

void Canceler::cancel(const State state) noexcept {
	const uint32_t value = 0x00000001u | (static_cast<uint32_t>(state) << 1u);
	m_state.store(value);
}

void Canceler::reset() noexcept {
	m_state.store(0x00000000u);
}

bool Canceler::isCanceled() const noexcept {
	return static_cast<bool>(m_state.load() & 0x00000001u);
}

uint32_t Canceler::getState() const noexcept {
	return m_state.load();
}

} // namespace Canceler
