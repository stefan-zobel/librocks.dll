
#include <thread>
#include "impl/StoreImpl.h"
#include "impl/SimpleLogger.h"

constexpr uint64_t DEFAULT_COMPACTION_MEMTABLE_MEMORY_BUDGET = 512L * 1024L * 1024L;

inline static KindImpl* toKindImpl(const Kind& k, int* status) {
    auto kind = dynamic_cast<KindImpl*>(const_cast<Kind*>(&k));
    if (!kind) {
        assign(InvalidArgument, status);
    }
    return kind;
}

StoreImpl::StoreImpl(const char* path_) : open(false), code(Ok), txnDb(nullptr), reusableTx_(nullptr),
    totalSinceLastSync(0L) {

    path.append(path_);
    open_();
}

StoreImpl::~StoreImpl() {
    if (open) {
        close();
    }
}

inline rocksdb::Transaction* StoreImpl::reusableTx() noexcept {
    if (!reusableTx_) {
        reusableTx_ = txnDb->BeginTransaction(writeOptions, txnOpts);
    }
    return reusableTx_;
}

inline void StoreImpl::deleteIfNewTx(rocksdb::Transaction* tx) const noexcept {
    if (tx != reusableTx_) {
        delete tx;
    }
}

void StoreImpl::open_() {
    options.create_if_missing = true;
    options.error_if_exists = false;
    options.keep_log_file_num = 2;
    options.delete_obsolete_files_period_micros = 3600000000L;
    options.WAL_ttl_seconds = 4L * 3600L;
    options.WAL_size_limit_MB = 64L;
    options.recycle_log_file_num = 10;
    options.IncreaseParallelism(std::max(std::thread::hardware_concurrency(), 2u));
    options.info_log_level = rocksdb::InfoLogLevel::WARN_LEVEL;
    txnDbOptions.write_policy = rocksdb::TxnDBWritePolicy::WRITE_COMMITTED;
    columnFamilyOptions.periodic_compaction_seconds = 1L * 24L * 60L * 60L;
    columnFamilyOptions.OptimizeLevelStyleCompaction(DEFAULT_COMPACTION_MEMTABLE_MEMORY_BUDGET);
    flushOptions.wait = true;
    flushOptionsNoWait.wait = false;
    rocksdb::Status s = openDatabase();
    if (s.ok()) {
        reusableTx_ = reusableTx();
        auto& build = rocksdb::GetRocksBuildProperties();
        version.append("RocksDB version: ").append(rocksdb::GetRocksVersionAsString());
        version.append(", Git sha: ").append(build.at("rocksdb_build_git_sha"));
        version.append(", Compiled: ").append(build.at("rocksdb_build_date"));
        __LOG_INFO __LARG(version);
        __LOG_INFO __LARG(std::string("Database opened at: ").append(path));
        open = true;
    }
    else {
        code = s.code();
    }
    lastSync = std::chrono::steady_clock::now();
}

rocksdb::Status StoreImpl::openDatabase() {
    std::vector<std::string> families;
    rocksdb::Status s = rocksdb::DB::ListColumnFamilies(options, path, &families);
    if (s.ok() || s.IsPathNotFound()) {
        if (families.empty()) {
            families.push_back(rocksdb::kDefaultColumnFamilyName);
        }
        std::vector<rocksdb::ColumnFamilyDescriptor> cfDescs;
        for (auto& cfName : families) {
            cfDescs.emplace_back(cfName, columnFamilyOptions);
        }
        std::vector<rocksdb::ColumnFamilyHandle*> cfHandles;
        s = rocksdb::TransactionDB::Open(options, txnDbOptions, path, cfDescs, &cfHandles, &txnDb);
        if (s.ok()) {
            for (size_t i = 0; i < cfDescs.size(); ++i) {
                kinds[cfDescs[i].name] = KindImpl(cfDescs[i].name.c_str(), cfHandles[i]);
            }
        }
    }
    return s;
}

void StoreImpl::close() {
    synchronize(monitor);
    if (open) {
        open = false;
        syncWALNoLock();
        flushNoLock(flushOptions);
        for (auto& it : kinds) {
            it.second.closeHandle();
        }
        kinds.clear();
        delete txnDb;
        txnDb = nullptr;
    }
}

inline bool StoreImpl::isOpen() const noexcept {
    return open;
}

int StoreImpl::getCode() const noexcept {
    return code;
}

inline bool StoreImpl::validateOpen(int* status) const noexcept {
    if (isOpen()) {
        assign(Ok, status);
        return true;
    }
    assign(Closed, status);
    code = Closed;
    return false;
}

KindManager& StoreImpl::getKindManager(int* status) const noexcept {
    validateOpen(status);
    return *const_cast<StoreImpl*>(this);
}

const Kind** StoreImpl::getKinds(int* status, size_t* resultLen) const noexcept {
    synchronize(monitor);
    if (validateOpen(status)) {
        size_t i = 0;
        size_t size = kinds.size();
        const Kind** ppK = new const Kind*[size];
        for (auto& it : kinds) {
            ppK[i] = &it.second;
            ++i;
        }
        *resultLen = size;
        return ppK;
    }
    else {
        assign(Closed, status);
        return nullptr;
    }
}

const Kind& StoreImpl::getDefaultKind(int* status) const noexcept {
    synchronize(monitor);
    if (validateOpen(status)) {
        if (auto kind = kinds.find(StoreImpl::DefaultCF); kind != kinds.end()) {
            return kind->second;
        }
        assign(NotFound, status);
    }
    return StoreImpl::EmptyKind;
}

const Kind& StoreImpl::getOrCreateKind(int* status, const char* kindName) noexcept {
    synchronize(monitor);
    if (validateOpen(status)) {
        if (auto kind = kinds.find(std::string(kindName)); kind != kinds.end()) {
            return kind->second;
        }
        else {
            return createKind(status, kindName);
        }
    }
    return StoreImpl::EmptyKind;
}

KindImpl& StoreImpl::createKind(int* status, const char* kindName) noexcept {
    rocksdb::ColumnFamilyHandle* cfHandle = nullptr;
    std::string cfName(kindName);
    rocksdb::Status s = txnDb->CreateColumnFamily(columnFamilyOptions, cfName, &cfHandle);
    if (s.ok()) {
        return kinds[cfName] = KindImpl(cfName.c_str(), cfHandle);
    }
    assign(s.code(), status);
    return const_cast<KindImpl&>(StoreImpl::EmptyKind);
}

void StoreImpl::put(int* status, const Kind& kind, const char* key, size_t keyLen, const char* value,
    size_t valLen) noexcept {

    if (!key || !kind.isValid()) {
        assign(InvalidArgument, status);
        return;
    }
    synchronize(monitor);
    if (validateOpen(status)) {
        put_(status, kind, key, keyLen, value, valLen);
    }
    else {
        assign(Closed, status);
    }
}

void StoreImpl::remove(int* status, const Kind& kind, const char* key, size_t keyLen) noexcept {
    if (!key || !kind.isValid()) {
        assign(InvalidArgument, status);
        return;
    }
    synchronize(monitor);
    if (validateOpen(status)) {
        remove_(status, kind, key, keyLen);
    }
    else {
        assign(Closed, status);
    }
}

void StoreImpl::removeRange(int* status, const Kind& k, const char* beginKeyInclusive, size_t beginKeyLen,
    const char* endKeyExclusive, size_t endKeyLen) noexcept {

    if (!beginKeyInclusive || !endKeyExclusive || !k.isValid()) {
        assign(InvalidArgument, status);
        return;
    }
    synchronize(monitor);
    if (validateOpen(status)) {
        KindImpl* kind = toKindImpl(k, status);
        if (!kind) {
            return;
        }
        rocksdb::TransactionDBWriteOptimizations optim;
        optim.skip_concurrency_control = true;
        rocksdb::WriteBatch wb;
        rocksdb::Status s = wb.DeleteRange(kind->handle(), rocksdb::Slice(beginKeyInclusive, beginKeyLen),
            rocksdb::Slice(endKeyExclusive, endKeyLen));
        if (s.ok()) {
            s = txnDb->Write(writeOptions, optim, &wb);
            if (!s.ok()) {
                assign(s.code(), status);
            }
        }
        else {
            assign(s.code(), status);
        }
    }
    else {
        assign(Closed, status);
    }
}

char* StoreImpl::get(int* status, const Kind& kind, size_t* resultLen, const char* key,
    size_t keyLen) const noexcept {

    if (!key || !resultLen || !kind.isValid()) {
        assign(InvalidArgument, status);
        return nullptr;
    }
    *resultLen = 0;
    synchronize(monitor);
    if (validateOpen(status)) {
        return get_(status, kind, resultLen, key, keyLen);
    }
    assign(Closed, status);
    return nullptr;
}

void StoreImpl::put_(int* status, const Kind& k, const char* key, size_t keyLen, const char* value,
    size_t valLen) noexcept {

    KindImpl* kind = toKindImpl(k, status);
    if (!kind) {
        return;
    }
    rocksdb::Transaction* tx = txnDb->BeginTransaction(writeOptions, txnOpts, reusableTx());
    if (tx) {
        rocksdb::Status s = tx->Put(kind->handle(), rocksdb::Slice(key, keyLen), rocksdb::Slice(value, valLen));
        if (s.ok()) {
            s = tx->Commit();
            if (s.ok()) {
                occasionalWalSync();
            }
            else {
                assign(s.code(), status);
            }
        }
        else {
            assign(s.code(), status);
        }
    }
    else {
        assign(NoTransaction, status);
    }
    deleteIfNewTx(tx);
}

void StoreImpl::remove_(int* status, const Kind& k, const char* key, size_t keyLen) noexcept {
    KindImpl* kind = toKindImpl(k, status);

    if (!kind) {
        return;
    }
    rocksdb::Transaction* tx = txnDb->BeginTransaction(writeOptions, txnOpts, reusableTx());
    if (tx) {
        rocksdb::Status s = tx->Delete(kind->handle(), rocksdb::Slice(key, keyLen));
        if (s.ok()) {
            s = tx->Commit();
            if (s.ok()) {
                occasionalWalSync();
            }
            else {
                assign(s.code(), status);
            }
        }
        else {
            assign(s.code(), status);
        }
    }
    else {
        assign(NoTransaction, status);
    }
    deleteIfNewTx(tx);
}

char* StoreImpl::get_(int* status, const Kind& k, size_t* resultLen, const char* key,
    size_t keyLen) const noexcept {

    KindImpl* kind = toKindImpl(k, status);
    if (!kind) {
        return nullptr;
    }
    rocksdb::PinnableSlice pinnable;
    rocksdb::Status s = txnDb->Get(readOptions, kind->handle(), rocksdb::Slice(key, keyLen), &pinnable);
    if (s.ok()) {
        if (!pinnable.empty()) {
            *resultLen = pinnable.size();
            char* data = new char[*resultLen];
            std::memcpy(data, pinnable.data(), *resultLen);
            return data;
        }
    }
    else {
        assign(s.code(), status);
    }
    return nullptr;
}

char* StoreImpl::updateIfPresent(int* status, const Kind& kind, size_t* resultLen, const char* key,
    size_t keyLen, const char* value, size_t valLen) noexcept {

    if (!key || !resultLen || !kind.isValid()) {
        assign(InvalidArgument, status);
        return nullptr;
    }
    *resultLen = 0;
    synchronize(monitor);
    if (validateOpen(status)) {
        int state = Ok;
        char* oldVal = get_(&state, kind, resultLen, key, keyLen);
        if (state == Ok) {
            put_(&state, kind, key, keyLen, value, valLen);
        }
        assign(state, status);
        return oldVal;
    }
    assign(Closed, status);
    return nullptr;
}

char* StoreImpl::singleRemoveIfPresent(int* status, const Kind& kind, size_t* resultLen,
    const char* key, size_t keyLen) noexcept {

    if (!key || !resultLen || !kind.isValid()) {
        assign(InvalidArgument, status);
        return nullptr;
    }
    *resultLen = 0;
    synchronize(monitor);
    if (validateOpen(status)) {
        int state = Ok;
        char* oldVal = get_(&state, kind, resultLen, key, keyLen);
        if (state == Ok) {
            singleRemove_(&state, kind, key, keyLen);
        }
        assign(state, status);
        return oldVal;
    }
    assign(Closed, status);
    return nullptr;
}

char* StoreImpl::removeIfPresent(int* status, const Kind& kind, size_t* resultLen, const char* key,
    size_t keyLen) noexcept {

    if (!key || !resultLen || !kind.isValid()) {
        assign(InvalidArgument, status);
        return nullptr;
    }
    *resultLen = 0;
    synchronize(monitor);
    if (validateOpen(status)) {
        int state = Ok;
        char* oldVal = get_(&state, kind, resultLen, key, keyLen);
        if (state == Ok) {
            remove_(&state, kind, key, keyLen);
        }
        assign(state, status);
        return oldVal;
    }
    assign(Closed, status);
    return nullptr;
}

void StoreImpl::singleRemove(int* status, const Kind& kind, const char* key, size_t keyLen) noexcept {
    if (!key || !kind.isValid()) {
        assign(InvalidArgument, status);
        return;
    }
    synchronize(monitor);
    if (validateOpen(status)) {
        singleRemove_(status, kind, key, keyLen);
    }
    else {
        assign(Closed, status);
    }
}

void StoreImpl::singleRemove_(int* status, const Kind& k, const char* key, size_t keyLen) noexcept {
    KindImpl* kind = toKindImpl(k, status);
    if (!kind) {
        return;
    }
    rocksdb::Transaction* tx = txnDb->BeginTransaction(writeOptions, txnOpts, reusableTx());
    if (tx) {
        rocksdb::Status s = tx->SingleDelete(kind->handle(), rocksdb::Slice(key, keyLen));
        if (s.ok()) {
            s = tx->Commit();
            if (s.ok()) {
                occasionalWalSync();
            }
            else {
                assign(s.code(), status);
            }
        }
        else {
            assign(s.code(), status);
        }
    }
    else {
        assign(NoTransaction, status);
    }
    deleteIfNewTx(tx);
}

void StoreImpl::putIfAbsent(int* status, const Kind& kind, const char* key, size_t keyLen, const char* value,
    size_t valLen) noexcept {

    if (!key || !kind.isValid()) {
        assign(InvalidArgument, status);
        return;
    }
    synchronize(monitor);
    if (validateOpen(status)) {
        size_t dummyLen;
        int state = Ok;
        get_(&state, kind, &dummyLen, key, keyLen);
        if (state == NotFound) {
            state = Ok;
            put_(&state, kind, key, keyLen, value, valLen);
        }
        else if (state == Ok) {
            state = AlreadyExists;
        }
        assign(state, status);
    }
    else {
        assign(Closed, status);
    }
}

void StoreImpl::syncWAL() noexcept {
    synchronize(monitor);
    syncWALNoLock();
}

void StoreImpl::flush() noexcept {
    synchronize(monitor);
    flushNoLock(flushOptions);
}

void StoreImpl::flushNoWait() noexcept {
    synchronize(monitor);
    flushNoLock(flushOptionsNoWait);
}

void StoreImpl::syncWALNoLock() noexcept {
    if (open) {
        txnDb->FlushWAL(true);
    }
}

void StoreImpl::flushNoLock(rocksdb::FlushOptions& flushOpts) noexcept {
    if (open) {
        txnDb->Flush(flushOpts);
    }
}

void StoreImpl::syncAndReset() noexcept {
    syncWALNoLock();
    lastSync = std::chrono::steady_clock::now();
    totalSinceLastSync = 0L;
}

void StoreImpl::occasionalWalSync() noexcept {
    ++totalSinceLastSync;
    if ((since(lastSync) >= FLUSH_TIME_WINDOW_MILLIS)
        || ((totalSinceLastSync & FLUSH_BATCH_SIZE) == 0L)) {
        syncAndReset();
    }
}

char* StoreImpl::findMinKey(int* status, const Kind& k, size_t* resultLen) const noexcept {
    if (!k.isValid()) {
        assign(InvalidArgument, status);
        return nullptr;
    }
    KindImpl* kind = toKindImpl(k, status);
    if (!kind) {
        return nullptr;
    }
    *resultLen = 0;
    synchronize(monitor);
    if (validateOpen(status)) {
        rocksdb::Iterator* it = txnDb->NewIterator(readOptions, kind->handle());
        if (it) {
            it->SeekToFirst();
            char* minKey = nullptr;
            if (it->Valid()) {
                rocksdb::Slice key = it->key();
                if (!key.empty()) {
                    *resultLen = key.size();
                    minKey = new char[*resultLen];
                    std::memcpy(minKey, key.data(), *resultLen);
                }
            }
            delete it;
            return minKey;
        }
        else {
            assign(NoIterator, status);
        }
    }
    else {
        assign(Closed, status);
    }
    return nullptr;
}

char* StoreImpl::findMaxKey(int* status, const Kind& k, size_t* resultLen) const noexcept {
    if (!k.isValid()) {
        assign(InvalidArgument, status);
        return nullptr;
    }
    KindImpl* kind = toKindImpl(k, status);
    if (!kind) {
        return nullptr;
    }
    *resultLen = 0;
    synchronize(monitor);
    if (validateOpen(status)) {
        rocksdb::Iterator* it = txnDb->NewIterator(readOptions, kind->handle());
        if (it) {
            it->SeekToLast();
            char* maxKey = nullptr;
            if (it->Valid()) {
                rocksdb::Slice key = it->key();
                if (!key.empty()) {
                    *resultLen = key.size();
                    maxKey = new char[*resultLen];
                    std::memcpy(maxKey, key.data(), *resultLen);
                }
            }
            delete it;
            return maxKey;
        }
        else {
            assign(NoIterator, status);
        }
    }
    else {
        assign(Closed, status);
    }
    return nullptr;
}

void StoreImpl::compact(int* status, const Kind& k) noexcept {
    if (!k.isValid()) {
        assign(InvalidArgument, status);
        return;
    }
    KindImpl* kind = toKindImpl(k, status);
    if (!kind) {
        return;
    }
    synchronize(monitor);
    if (validateOpen(status)) {
        compactKind(status, k);
    }
    else {
        assign(Closed, status);
    }
}

void StoreImpl::compactAll(int* status) noexcept {
    synchronize(monitor);
    if (validateOpen(status)) {
        for (auto& it : kinds) {
            if (!compactKind(status, it.second)) {
                break;
            }
        }
    }
    else {
        assign(Closed, status);
    }
}

bool StoreImpl::compactKind(int* status, const Kind& kind) noexcept {
    const KindImpl& impl = dynamic_cast<const KindImpl&>(kind);
    rocksdb::ColumnFamilyHandle* cfHandle = impl.handle();
    rocksdb::Status s = txnDb->CompactRange(rocksdb::CompactRangeOptions(), cfHandle, nullptr, nullptr);
    if (s.ok()) {
        return true;
    }
    else {
        assign(s.code(), status);
        return false;
    }
}
