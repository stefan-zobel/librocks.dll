#pragma once

#include <string>
#include <unordered_map>
#include "api/Store.h"
#include "impl/utils.h"
#include "impl/KindImpl.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

class StoreImpl : public Store, public KindManager {
public:
    explicit StoreImpl(const char* path);

    void close() override final;

    bool isOpen() const noexcept override;

    int getCode() const noexcept;

    KindManager& getKindManager(int* status) const noexcept override;

    const Kind& getDefaultKind(int* status) const noexcept override;

    const Kind& getOrCreateKind(int* status, const char* kindName) noexcept override;

    const Kind** getKinds(int* status, size_t* resultLen) const noexcept override;

    void put(int* status, const Kind& kind, const char* key, size_t keyLen, const char* value,
        size_t valLen) noexcept override;

    void remove(int* status, const Kind& kind, const char* key, size_t keyLen) noexcept override;

    char* get(int* status, const Kind& kind, size_t* resultLen, const char* key, size_t keyLen)
        const noexcept override;

    char* updateIfPresent(int* status, const Kind& kind, size_t* resultLen, const char* key,
        size_t keyLen, const char* value, size_t valLen) noexcept override;

    void singleRemove(int* status, const Kind& kind, const char* key, size_t keyLen) noexcept override;

    char* singleRemoveIfPresent(int* status, const Kind& kind, size_t* resultLen, const char* key,
        size_t keyLen) noexcept override;

    char* removeIfPresent(int* status, const Kind& kind, size_t* resultLen, const char* key,
        size_t keyLen) noexcept override;

    void putIfAbsent(int* status, const Kind& kind, const char* key, size_t keyLen, const char* value,
        size_t valLen) noexcept override;

    char* findMinKey(int* status, const Kind& kind, size_t* resultLen) const noexcept override;

    char* findMaxKey(int* status, const Kind& kind, size_t* resultLen) const noexcept override;

    void syncWAL() noexcept override;

    void flush() noexcept override;

    void flushNoWait() noexcept override;

    void compact(int* status, const Kind& kind) noexcept override;

    void compactAll(int* status) noexcept override;

    void removeRange(int* status, const Kind& kind, const char* beginKeyInclusive, size_t beginKeyLen,
        const char* endKeyExclusive, size_t endKeyLen) noexcept override;

    ~StoreImpl() override;

private:
    void open_();
    rocksdb::Status openDatabase();
    bool validateOpen(int* status) const noexcept;
    void syncWALNoLock() noexcept;
    void flushNoLock(rocksdb::FlushOptions&) noexcept;
    void occasionalWalSync() noexcept;
    void syncAndReset() noexcept;
    rocksdb::Transaction* reusableTx() noexcept;
    void deleteIfNewTx(rocksdb::Transaction* tx) const noexcept;
    KindImpl& createKind(int* status, const char* kindName) noexcept;
    void put_(int* status, const Kind& kind, const char* key, size_t keyLen, const char* value,
        size_t valLen) noexcept;
    void remove_(int* status, const Kind& kind, const char* key, size_t keyLen) noexcept;
    char* get_(int* status, const Kind& kind, size_t* resultLen, const char* key, size_t keyLen)
        const noexcept;
    void singleRemove_(int* status, const Kind& kind, const char* key, size_t keyLen) noexcept;
    bool compactKind(int* status, const Kind& kind) noexcept;

private:
    bool open;
    mutable std::mutex monitor;
    rocksdb::DBOptions options;
    rocksdb::TransactionDBOptions txnDbOptions;
    rocksdb::TransactionOptions txnOpts;
    rocksdb::ColumnFamilyOptions columnFamilyOptions;
    rocksdb::WriteOptions writeOptions;
    rocksdb::ReadOptions readOptions;
    rocksdb::FlushOptions flushOptions;
    rocksdb::FlushOptions flushOptionsNoWait;
    rocksdb::TransactionDB* txnDb;
    rocksdb::Transaction* reusableTx_;
    rocksdb::SstFileManager* sstFileManager;
    std::unordered_map<std::string, KindImpl> kinds;
    std::string path;
    std::string version;
    mutable int code;
    uint64_t totalSinceLastSync;
    std::chrono::steady_clock::time_point lastSync;

private:
    inline static const uint64_t FLUSH_BATCH_SIZE = 16384L - 1L;
    inline static const std::chrono::milliseconds FLUSH_TIME_WINDOW_MILLIS{985L};
    inline static const std::string DefaultCF { "default" };
    inline static const KindImpl EmptyKind = KindImpl();
};
