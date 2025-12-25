#pragma once
#include <pqxx/pqxx>
#include "../db.hpp"

// RAII-обертка над pqxx::work.
// - Если commit() не вызван, транзакция будет отменена при разрушении объекта.
class UnitOfWork {
public:
    explicit UnitOfWork(Db& db)
        : db_(db), tx_(db_.conn()) {}

    pqxx::work& tx() { return tx_; }

    void commit() {
        tx_.commit();
        committed_ = true;
    }

    ~UnitOfWork() {
        // pqxx::work сам отменит транзакцию, если commit() не был вызван,
        // но делаем это явно и безопасно.
        if (!committed_) {
            try { tx_.abort(); } catch (...) {}
        }
    }

    UnitOfWork(const UnitOfWork&) = delete;
    UnitOfWork& operator=(const UnitOfWork&) = delete;

private:
    Db& db_;
    pqxx::work tx_;
    bool committed_ = false;
};
