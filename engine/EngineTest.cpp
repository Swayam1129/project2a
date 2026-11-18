// Unit tests for the EntityManager (the core of the ECS engine).
// Build target: ECSTest. Run it with no arguments; it prints each check and exits with 1 if
// any check fails.
#include <cstdio>
#include <string>

#include "EntityManager.h"

namespace {

struct Position {
    int x, y;
};
struct Health {
    int hp;
};
using Manager = ECSEngine::EntityManager<Position, Health>;

int gFailures = 0;

void Check(bool ok, const char* what) {
    std::printf("%s  %s\n", ok ? "[pass]" : "[FAIL]", what);
    if (!ok) ++gFailures;
}

int CountLive(const Manager& em) {
    int live = 0;
    for (auto it = em.cbegin(); it != em.cend(); ++it)
        if (em.IsValid(it->GetID())) ++live;
    return live;
}

void TestCreate() {
    Manager em;
    ECSEngine::EntityID a = em.CreateEntity("a");
    ECSEngine::EntityID b = em.CreateEntity("b");
    ECSEngine::EntityID c = em.CreateEntity("c");
    Check(a == 0 && b == 1 && c == 2, "new entities get ids 0, 1, 2");
    Check(em.IsValid(a) && em.IsValid(b) && em.IsValid(c), "new entities are valid");
    Check(!em.IsValid(3) && !em.IsValid(1000), "ids that were never created are invalid");
    Check(CountLive(em) == 3, "iteration sees 3 live entities");
}

void TestRemoveAndReuse() {
    Manager em;
    ECSEngine::EntityID a = em.CreateEntity("a");
    ECSEngine::EntityID b = em.CreateEntity("b");
    ECSEngine::EntityID c = em.CreateEntity("c");
    em.RemoveEntity(b);
    Check(!em.IsValid(b), "removed entity is invalid");
    Check(em.IsValid(a) && em.IsValid(c), "other entities keep their ids");
    Check(CountLive(em) == 2, "iteration skips the removed entity");
    em.RemoveEntity(b);  // removing twice must be harmless
    Check(CountLive(em) == 2, "removing twice does nothing");
    ECSEngine::EntityID d = em.CreateEntity("d");
    Check(d == b && em.IsValid(d), "a removed id is reused by the next CreateEntity");
    ECSEngine::EntityID e = em.CreateEntity("e");
    Check(e == 3, "with no free ids, CreateEntity appends");
}

void TestComponents() {
    Manager em;
    ECSEngine::EntityID a = em.CreateEntity("a");
    ECSEngine::EntityID b = em.CreateEntity("b");
    em.AddComponent(a, Position{1, 2});
    em.AddComponent(b, Health{50});
    Check(em.HasComponent<Position>(a) && !em.HasComponent<Health>(a), "a has only Position");
    Check(em.HasComponent<Health>(b) && !em.HasComponent<Position>(b), "b has only Health");
    Check(em.GetComponent<Position>(a).x == 1 && em.GetComponent<Position>(a).y == 2,
          "GetComponent returns the stored value");

    em.GetComponent<Position>(a).x = 7;
    Check(em.GetComponent<Position>(a).x == 7, "GetComponent returns a reference that can be edited");

    em.AddComponent(a, Position{9, 9});
    Check(em.GetComponent<Position>(a).x == 9, "adding a component twice overwrites it");
    Check(em.GetComponentStorage<Position>().mStorage.size() == 1, "...without using a new slot");

    em.RemoveComponent<Position>(a);
    Check(!em.HasComponent<Position>(a), "RemoveComponent removes it");
    em.RemoveComponent<Position>(a);  // harmless
    Check(!em.HasComponent<Position>(a), "removing a missing component does nothing");

    em.AddComponent(b, Position{3, 4});
    Check(em.GetComponentStorage<Position>().mStorage.size() == 1,
          "a freed component slot is reused");
    Check(em.GetComponent<Position>(b).x == 3, "reused slot holds the new value");
}

void TestRemoveEntityClearsComponents() {
    Manager em;
    ECSEngine::EntityID a = em.CreateEntity("a");
    em.AddComponent(a, Position{1, 1});
    em.AddComponent(a, Health{10});
    em.RemoveEntity(a);
    Check(!em.HasComponent<Position>(a) && !em.HasComponent<Health>(a),
          "RemoveEntity removes all of its components");
    ECSEngine::EntityID b = em.CreateEntity("b");
    Check(b == a && !em.HasComponent<Position>(b) && !em.HasComponent<Health>(b),
          "a reused id starts with no components");
}

void TestStorageValidFlags() {
    Manager em;
    ECSEngine::EntityID a = em.CreateEntity("a");
    ECSEngine::EntityID b = em.CreateEntity("b");
    em.AddComponent(a, Health{1});
    em.AddComponent(b, Health{2});
    em.RemoveComponent<Health>(a);
    auto& storage = em.GetComponentStorage<Health>();
    int valid = 0;
    for (size_t i = 0; i < storage.mStorage.size(); ++i)
        if (storage.mValid[i]) ++valid;
    Check(valid == 1, "storage marks removed components invalid");
}

}  // namespace

int main() {
    TestCreate();
    TestRemoveAndReuse();
    TestComponents();
    TestRemoveEntityClearsComponents();
    TestStorageValidFlags();
    std::printf(gFailures == 0 ? "All EntityManager tests passed.\n"
                               : "%d EntityManager test(s) FAILED.\n",
                gFailures);
    return gFailures == 0 ? 0 : 1;
}
