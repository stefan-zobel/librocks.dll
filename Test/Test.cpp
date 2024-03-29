
#include "api/librocks.h"
#include "client/KVStore.h"
#include "client/RocksException.h"
#include "client/bytes.h"
#include <exception>
#include <string>
#include <iostream>


int main() {

    try {
        int status = Ok;
        KVStore kv(openStore(&status, ".\\rocksdb_database"));
        if (status != Ok) {
            std::cout << "Open failed!\n";
            return -1;
        }

        kv.compactAll();

        for (auto& kind : kv.getKinds()) {
            std::cout << kind.get().name() << std::endl;
        }

        std::string kindName1 = "ABCDEF";
        const Kind& newKind = kv.getOrCreateKind(kindName1);

        std::string newKey = "newKey_0";
        std::string newValue = "newValue_0";
        kv.put(newKind, newKey, newValue);

        std::string newKey2a = "newKey_2a";
        std::string newKey2b = "newKey_2b";
        std::string newKey2c = "newKey_2c";
        std::string newKey2d = "newKey_2d";
        kv.remove(newKind, newKey2a);

        kv.singleRemove(newKind, newKey2b);
        bytes removed1 = kv.singleRemoveIfPresent(newKind, newKey2c);
        std::cout << "removed1 is empty : " << removed1.isEmpty() << std::endl;
        bytes removed2 = kv.removeIfPresent(newKind, newKey2d);
        std::cout << "removed2 is empty : " << removed2.isEmpty() << std::endl;

        std::string newKey3 = "newKey_3";
        bytes result = kv.get(newKind, newKey3);
        std::cout << "result is empty : " << result.isEmpty() << std::endl;

        std::string newKey4 = "newKey_4";
        std::string newValue4 = "ABC";
        std::string newValue5 = "XYZ";

        bytes result2 = kv.updateIfPresent(newKind, newKey4, newValue4);
        std::cout << "result2 is empty : " << result2.isEmpty() << std::endl;
        bytes result3 = kv.updateIfPresent(newKind, newKey4, newValue5);
        std::cout << "result3 is empty : " << result2.isEmpty() << std::endl;

        bool wasAbsent = kv.putIfAbsent(newKind, newKey4, newValue4);
        std::cout << "wasAbsent: " << wasAbsent << std::endl;

        std::string kindName2 = "GHIJKL";
        const Kind& newKind2 = kv.getOrCreateKind(kindName2);
        bytes minKey = kv.findMinKey(newKind2);
        bytes maxKey = kv.findMaxKey(newKind2);

        kv.close();

        Store* store = openStore(&status, ".\\rocksdb_database");
        status = Ok;
        const Kind& theKind = store->getKindManager(&status).getOrCreateKind(&status, "ABCDEF");
        store->put(&status, theKind, "a", 1, "val1", 4);
        store->put(&status, theKind, "b", 1, "val2", 4);
        store->put(&status, theKind, "c", 1, "val3", 4);
        store->removeRange(&status, theKind, "a", 1, "d", 1);
        size_t resSize = 0;
        char* val = store->get(&status, theKind, &resSize, "a", 1);
        delete[] val;
        store->close();
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what();
    }
    std::cout << "Finished\n";
    return 0;
}
