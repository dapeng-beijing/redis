set testmodule [file normalize tests/modules/blockonbackground.so]

source tests/support/util.tcl

start_server {tags {"modules"}} {
    r module load $testmodule

    test { blocked clients time tracking - check blocked command that uses RedisModule_BlockedClientMeasureTimeStart() is tracking background time} {
        r slowlog reset
        r config set slowlog-log-slower-than 200000
        assert_equal [r slowlog len] 0
        r block.debug 0 10000
        assert_equal [r slowlog len] 0
        r config resetstat
        r block.debug 200 10000
        assert_equal [r slowlog len] 1

        set cmdstatline [cmdrstat block.debug r]

        regexp "calls=1,usec=(.*?),usec_per_call=(.*?),rejected_calls=0,failed_calls=0" $cmdstatline usec usec_per_call
        assert {$usec >= 100000}
        assert {$usec_per_call >= 100000}
    }

    test { blocked clients time tracking - check blocked command that uses RedisModule_BlockedClientMeasureTimeStart() is tracking background time even in timeout } {
        r slowlog reset
        r config set slowlog-log-slower-than 200000
        assert_equal [r slowlog len] 0
        r block.debug 0 20000
        assert_equal [r slowlog len] 0
        r config resetstat
        r block.debug 20000 500
        assert_equal [r slowlog len] 1

        set cmdstatline [cmdrstat block.debug r]

        regexp "calls=1,usec=(.*?),usec_per_call=(.*?),rejected_calls=0,failed_calls=0" $cmdstatline usec usec_per_call
        assert {$usec >= 250000}
        assert {$usec_per_call >= 250000}
    }

    test { blocked clients time tracking - check blocked command with multiple calls RedisModule_BlockedClientMeasureTimeStart()  is tracking the total background time } {
        r slowlog reset
        r config set slowlog-log-slower-than 200000
        assert_equal [r slowlog len] 0
        r block.double_debug 0
        assert_equal [r slowlog len] 0
        r config resetstat
        r block.double_debug 100
        assert_equal [r slowlog len] 1

        set cmdstatline [cmdrstat block.double_debug r]

        regexp "calls=1,usec=(.*?),usec_per_call=(.*?),rejected_calls=0,failed_calls=0" $cmdstatline usec usec_per_call
        assert {$usec >= 60000}
        assert {$usec_per_call >= 60000}
    }

    test { blocked clients time tracking - check blocked command without calling RedisModule_BlockedClientMeasureTimeStart() is not reporting background time } {
        r slowlog reset
        r config set slowlog-log-slower-than 200000
        assert_equal [r slowlog len] 0
        r block.debug_no_track 200 1000
        # ensure slowlog is still empty
        assert_equal [r slowlog len] 0
    }
}
