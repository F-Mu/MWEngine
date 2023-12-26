#pragma once

#include "core/base/hash.h"
#include <unordered_map>
#include <list>
#include <mutex>

namespace MW {
    static const size_t invalidGuid = 0;

    template<typename T>
    class GuidAllocator {
    public:
        static bool isValidGuid(size_t guid) { return guid != invalidGuid; }

        size_t allocGuid(const T &t) {
            auto find_it = elementsToGuidMap.find(t);
            if (find_it != elementsToGuidMap.end())
                return find_it->second;
            return allocateNewGuid();
        }

        bool getGuidRelatedElement(size_t guid, T &t) {
            auto find_it = guidToElementsMap.find(guid);
            if (find_it != guidToElementsMap.end()) {
                t = find_it->second;
                return true;
            }
            return false;
        }

        bool getElementGuid(const T &t, size_t &guid) {
            auto find_it = elementsToGuidMap.find(t);
            if (find_it != elementsToGuidMap.end()) {
                guid = find_it->second;
                return true;
            }
            return false;
        }

        bool hasElement(const T &t) { return elementsToGuidMap.find(t) != elementsToGuidMap.end(); }

        void freeGuid(size_t guid) {
            auto find_it = guidToElementsMap.find(guid);
            if (find_it != guidToElementsMap.end()) {
                const auto &ele = find_it->second;
                elementsToGuidMap.erase(ele);
                guidToElementsMap.erase(guid);
                restIdList.emplace_back(guid);
            }
        }

        void freeElement(const T &t) {
            auto find_it = elementsToGuidMap.find(t);
            if (find_it != elementsToGuidMap.end()) {
                const auto &guid = find_it->second;
                elementsToGuidMap.erase(t);
                guidToElementsMap.erase(guid);
                restIdList.emplace_back(guid);
            }
        }

        std::vector<size_t> getAllocatedGuids() const {
            std::vector<size_t> allocated_guids;
            for (const auto &ele: guidToElementsMap) {
                allocated_guids.push_back(ele.first);
            }
            return allocated_guids;
        }

        void clear() {
            elementsToGuidMap.clear();
            guidToElementsMap.clear();
        }

    private:
        int allocateNewGuid() {
            std::lock_guard lock(mutex);
            int Guid = restIdList.back();
            restIdList.pop_back();
            if (restIdList.empty())
                restIdList.emplace_back(Guid + 1);
            return Guid;
        }

        std::unordered_map<T, size_t> elementsToGuidMap;
        std::unordered_map<size_t, T> guidToElementsMap;
        std::list<size_t> restIdList{1};
        std::mutex mutex;
    };

} // namespace MW
