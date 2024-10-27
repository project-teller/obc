#include "modules/storage.h"
#include "tasks/storage.h"

[[noreturn]] void teller::tasks::storageReaderTask(void* arg)
{
    teller::storage::setup();
    teller::storage::runStorageReader();
}
