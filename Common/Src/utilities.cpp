#include <string>

std::string repeat(char ch, int count) {
    return count > 0 ? std::string(static_cast<std::size_t>(count), ch) : "";
}

std::string clipped(std::string_view text, int width) {
    if (width <= 0) {
        return {};
    }

    if (static_cast<int>(text.size()) <= width) {
        return std::string(text);
    }

    return std::string(text.substr(0, static_cast<std::size_t>(width)));
}