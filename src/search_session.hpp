#pragma once
#include <map>
#include <string>
#include <vector>

struct Threat;

class SearchSession {

public:
    int add(const std::vector<Threat>& items);
    bool export_csv(const std::string& filepath, std::string& error) const;
    std::size_t size() const { return items_.size(); }

private:
    std::map<std::string, Threat> items_;
};