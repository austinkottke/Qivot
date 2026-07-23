# Build all examples:
#   tutorial1-5, index - ORM basics: models, queries, joins, indexes
#   schema             - string/composite keys, CHECK, enums, FK cascade,
#                        has-many relations, bulk update, column migrations
#   jsonhttp           - fetch JSON from an HTTP/REST API on a worker thread
#                        and map it into models (offline via a local server)

TEMPLATE = subdirs

SUBDIRS += \
    tutorial1 \
    tutorial2 \
    tutorial3 \
    tutorial4 \
    tutorial5 \
    schema \
    migrations \
    relations \
    keyset \
    asyncquery \
    manytomany \
    dashboard \
    prefetch \
    savepoints \
    jsonnested \
    jsonhttp \
    qmlmodel \
    reactive \
    infinitescroll \
    contacts \
    index
