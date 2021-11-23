add_library(nan "")

target_sources(nan PRIVATE
        attributes.h
        attributes.c
        channel.h
        channel.c
        circular_buffer.h
        circular_buffer.c
        cluster.h
        cluster.c
        crc32.h
        crc32.c
        event.h
        event.c
        frame.h
        frame.c
        ieee80211.h
        ieee80211.c
        list.h
        list.c
        log.h
        log.c
        peer.h
        peer.c
        rx.h
        rx.c
        service.h
        service.c
        sha256.h
        sha256.c
        state.h
        state.c
        sync.h
        sync.c
        timer.h
        timer.c
        tx.h
        tx.c
        utils.h
        utils.c
        wire.h
        wire.c
       )

target_include_directories(nan PRIVATE ${CMAKE_SOURCE_DIR}/radiotap)

target_link_libraries(nan radiotap)