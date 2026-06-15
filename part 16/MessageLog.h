#pragma once
#include <string>
#include <vector>

// The on-screen combat/event log — the small extraction we earmarked at the
// Interlude. The game pushes lines; the renderer shows the most recent few.
class MessageLog
{
public:
    void add(const std::string& line)
    {
        m_lines.push_back(line);
        if (m_lines.size() > 200) m_lines.erase(m_lines.begin());   // cap memory
    }

    void clear() { m_lines.clear(); }

    const std::vector<std::string>& lines() const { return m_lines; }

private:
    std::vector<std::string> m_lines;
};
