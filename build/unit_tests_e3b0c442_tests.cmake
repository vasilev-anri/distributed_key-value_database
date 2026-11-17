add_test([=[UniqueFD.DefaultConstructedIsInvalid]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=UniqueFD.DefaultConstructedIsInvalid]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UniqueFD.DefaultConstructedIsInvalid]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/unique_fd_test.cpp:35]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UniqueFD.WrapsDescriptor]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=UniqueFD.WrapsDescriptor]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UniqueFD.WrapsDescriptor]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/unique_fd_test.cpp:42]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UniqueFD.DestructorClosesDescriptor]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=UniqueFD.DestructorClosesDescriptor]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UniqueFD.DestructorClosesDescriptor]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/unique_fd_test.cpp:52]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UniqueFD.MoveConstructorTransfersOwnership]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=UniqueFD.MoveConstructorTransfersOwnership]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UniqueFD.MoveConstructorTransfersOwnership]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/unique_fd_test.cpp:63]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UniqueFD.MoveAssignmentClosesOldAndTakesNew]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=UniqueFD.MoveAssignmentClosesOldAndTakesNew]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UniqueFD.MoveAssignmentClosesOldAndTakesNew]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/unique_fd_test.cpp:79]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UniqueFD.SelfMoveAssignmentKeepsDescriptor]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=UniqueFD.SelfMoveAssignmentKeepsDescriptor]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UniqueFD.SelfMoveAssignmentKeepsDescriptor]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/unique_fd_test.cpp:94]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UniqueFD.ReleaseGivesUpOwnershipWithoutClosing]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=UniqueFD.ReleaseGivesUpOwnershipWithoutClosing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UniqueFD.ReleaseGivesUpOwnershipWithoutClosing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/unique_fd_test.cpp:107]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UniqueFD.ResetWithoutArgumentCloses]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=UniqueFD.ResetWithoutArgumentCloses]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UniqueFD.ResetWithoutArgumentCloses]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/unique_fd_test.cpp:124]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UniqueFD.ResetWithNewDescriptorClosesOld]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=UniqueFD.ResetWithNewDescriptorClosesOld]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UniqueFD.ResetWithNewDescriptorClosesOld]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/unique_fd_test.cpp:134]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UniqueFD.ResetWithSameDescriptorDoesNotClose]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=UniqueFD.ResetWithSameDescriptorDoesNotClose]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UniqueFD.ResetWithSameDescriptorDoesNotClose]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/unique_fd_test.cpp:146]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ByteIO.WritesBigEndian]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ByteIO.WritesBigEndian]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ByteIO.WritesBigEndian]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:34]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ByteIO.ReaderNeverReadsPastEnd]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ByteIO.ReaderNeverReadsPastEnd]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ByteIO.ReaderNeverReadsPastEnd]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:46]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.HeaderStartsWithMagicAndHasFixedSize]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.HeaderStartsWithMagicAndHasFixedSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.HeaderStartsWithMagicAndHasFixedSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:57]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.PutRequestRoundTrip]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.PutRequestRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.PutRequestRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:65]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.GetResponseWithStatusAndValue]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.GetResponseWithStatusAndValue]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.GetResponseWithStatusAndValue]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:82]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.NotFoundResponseHasNoPayload]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.NotFoundResponseHasNoPayload]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.NotFoundResponseHasNoPayload]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:93]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.ForwardedFlagSurvivesRoundTrip]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.ForwardedFlagSurvivesRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.ForwardedFlagSurvivesRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:101]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.HelloRoundTrip]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.HelloRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.HelloRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:109]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.LargestPutFitsInOneDatagram]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.LargestPutFitsInOneDatagram]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.LargestPutFitsInOneDatagram]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:122]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.EncodingOversizedKeyThrows]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.EncodingOversizedKeyThrows]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.EncodingOversizedKeyThrows]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:131]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.TcpWaitsForEveryMissingByte]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.TcpWaitsForEveryMissingByte]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.TcpWaitsForEveryMissingByte]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:138]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.TcpParsesTwoMessagesFromOneBuffer]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.TcpParsesTwoMessagesFromOneBuffer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.TcpParsesTwoMessagesFromOneBuffer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:150]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.RejectsBadMagicImmediately]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.RejectsBadMagicImmediately]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.RejectsBadMagicImmediately]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:164]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.RejectsWrongVersion]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.RejectsWrongVersion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.RejectsWrongVersion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:171]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.RejectsUnknownType]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.RejectsUnknownType]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.RejectsUnknownType]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:177]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.RejectsInvalidStatus]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.RejectsInvalidStatus]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.RejectsInvalidStatus]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:183]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.RejectsHugeLengthFromHeaderAlone]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.RejectsHugeLengthFromHeaderAlone]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.RejectsHugeLengthFromHeaderAlone]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:189]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.DatagramMustNotBeTruncated]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.DatagramMustNotBeTruncated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.DatagramMustNotBeTruncated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:199]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.DatagramMustNotHaveTrailingBytes]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.DatagramMustNotHaveTrailingBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.DatagramMustNotHaveTrailingBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:206]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.PayloadWithTrailingBytesIsMalformed]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.PayloadWithTrailingBytesIsMalformed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.PayloadWithTrailingBytesIsMalformed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:212]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.PayloadLengthLargerThanDataIsMalformed]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.PayloadLengthLargerThanDataIsMalformed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.PayloadLengthLargerThanDataIsMalformed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:218]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.OversizedKeyIsRejectedWhenDecoding]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.OversizedKeyIsRejectedWhenDecoding]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.OversizedKeyIsRejectedWhenDecoding]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:225]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.StatusRoundTrip]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.StatusRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.StatusRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:234]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.LocateRoundTrip]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.LocateRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.LocateRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:249]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.LargestStatusFitsInOneDatagram]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.LargestStatusFitsInOneDatagram]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.LargestStatusFitsInOneDatagram]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:259]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.StatusWithInvalidNodeStateIsMalformed]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.StatusWithInvalidNodeStateIsMalformed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.StatusWithInvalidNodeStateIsMalformed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:271]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Codec.StatusWithTruncatedNodeListIsMalformed]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Codec.StatusWithTruncatedNodeListIsMalformed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Codec.StatusWithTruncatedNodeListIsMalformed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/protocol_test.cpp:278]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[KeyValueStore.GetMissingKeyReturnsNothing]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=KeyValueStore.GetMissingKeyReturnsNothing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[KeyValueStore.GetMissingKeyReturnsNothing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/kv_store_test.cpp:7]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[KeyValueStore.PutThenGet]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=KeyValueStore.PutThenGet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[KeyValueStore.PutThenGet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/kv_store_test.cpp:13]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[KeyValueStore.PutOverwritesExistingValue]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=KeyValueStore.PutOverwritesExistingValue]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[KeyValueStore.PutOverwritesExistingValue]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/kv_store_test.cpp:20]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[KeyValueStore.RemoveReportsWhetherKeyExisted]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=KeyValueStore.RemoveReportsWhetherKeyExisted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[KeyValueStore.RemoveReportsWhetherKeyExisted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/kv_store_test.cpp:28]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[KeyValueStore.ValuesMayContainAnyBytes]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=KeyValueStore.ValuesMayContainAnyBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[KeyValueStore.ValuesMayContainAnyBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/kv_store_test.cpp:36]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RequestHandlerTest.PutThenGetReturnsValue]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=RequestHandlerTest.PutThenGetReturnsValue]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RequestHandlerTest.PutThenGetReturnsValue]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/request_handler_test.cpp:57]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RequestHandlerTest.ResponseCopiesRequestIdentity]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=RequestHandlerTest.ResponseCopiesRequestIdentity]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RequestHandlerTest.ResponseCopiesRequestIdentity]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/request_handler_test.cpp:71]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RequestHandlerTest.GetMissingKeyIsNotFound]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=RequestHandlerTest.GetMissingKeyIsNotFound]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RequestHandlerTest.GetMissingKeyIsNotFound]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/request_handler_test.cpp:82]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RequestHandlerTest.PutOverwrites]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=RequestHandlerTest.PutOverwrites]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RequestHandlerTest.PutOverwrites]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/request_handler_test.cpp:89]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RequestHandlerTest.DeleteRemovesKey]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=RequestHandlerTest.DeleteRemovesKey]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RequestHandlerTest.DeleteRemovesKey]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/request_handler_test.cpp:97]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RequestHandlerTest.MalformedPayloadIsInvalidRequest]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=RequestHandlerTest.MalformedPayloadIsInvalidRequest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RequestHandlerTest.MalformedPayloadIsInvalidRequest]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/request_handler_test.cpp:109]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RequestHandlerTest.OversizedValueIsReported]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=RequestHandlerTest.OversizedValueIsReported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RequestHandlerTest.OversizedValueIsReported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/request_handler_test.cpp:116]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RequestHandlerTest.NonRequestMessagesAreIgnored]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=RequestHandlerTest.NonRequestMessagesAreIgnored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RequestHandlerTest.NonRequestMessagesAreIgnored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/request_handler_test.cpp:130]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Hash.Fnv1aMatchesReferenceValues]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Hash.Fnv1aMatchesReferenceValues]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Hash.Fnv1aMatchesReferenceValues]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/membership_test.cpp:30]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Hash.IsDeterministicAndSpreads]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Hash.IsDeterministicAndSpreads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Hash.IsDeterministicAndSpreads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/membership_test.cpp:38]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Membership.NodeIdComesFromCanonicalAddress]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Membership.NodeIdComesFromCanonicalAddress]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Membership.NodeIdComesFromCanonicalAddress]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/membership_test.cpp:47]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Membership.RejectsDuplicateNodes]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Membership.RejectsDuplicateNodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Membership.RejectsDuplicateNodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/membership_test.cpp:55]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Membership.ClosestNodeUsesXorDistance]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Membership.ClosestNodeUsesXorDistance]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Membership.ClosestNodeUsesXorDistance]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/membership_test.cpp:65]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Membership.AllNodesAgreeOnOwners]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Membership.AllNodesAgreeOnOwners]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Membership.AllNodesAgreeOnOwners]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/membership_test.cpp:75]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Membership.KeysSpreadOverAllNodes]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Membership.KeysSpreadOverAllNodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Membership.KeysSpreadOverAllNodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/membership_test.cpp:88]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Membership.ReplicaSetIsTheClosestNodesInOrder]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Membership.ReplicaSetIsTheClosestNodesInOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Membership.ReplicaSetIsTheClosestNodesInOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/membership_test.cpp:100]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Membership.AllNodesAgreeOnReplicaSets]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Membership.AllNodesAgreeOnReplicaSets]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Membership.AllNodesAgreeOnReplicaSets]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/membership_test.cpp:114]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Membership.SmallClusterHasFewerReplicas]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Membership.SmallClusterHasFewerReplicas]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Membership.SmallClusterHasFewerReplicas]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/membership_test.cpp:125]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.RequestToOwnerIsHandledLocally]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.RequestToOwnerIsHandledLocally]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.RequestToOwnerIsHandledLocally]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:229]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.PutThroughNonOwnerLandsOnOwner]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.PutThroughNonOwnerLandsOnOwner]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.PutThroughNonOwnerLandsOnOwner]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:241]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.AnyNodeCanReadAnyKey]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.AnyNodeCanReadAnyKey]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.AnyNodeCanReadAnyKey]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:256]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.ForwardedRequestIsNeverForwardedAgain]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.ForwardedRequestIsNeverForwardedAgain]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.ForwardedRequestIsNeverForwardedAgain]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:268]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.ForwardedWriteToReplicaIsRefused]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.ForwardedWriteToReplicaIsRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.ForwardedWriteToReplicaIsRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:278]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.ForwardedReadToReplicaIsServed]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.ForwardedReadToReplicaIsServed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.ForwardedReadToReplicaIsServed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:288]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.DeadOwnerGivesUnavailableAfterTimeout]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.DeadOwnerGivesUnavailableAfterTimeout]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.DeadOwnerGivesUnavailableAfterTimeout]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:296]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.ClientRetryIsNotForwardedTwice]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.ClientRetryIsNotForwardedTwice]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.ClientRetryIsNotForwardedTwice]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:311]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.ResponseFromWrongNodeIsIgnored]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.ResponseFromWrongNodeIsIgnored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.ResponseFromWrongNodeIsIgnored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:326]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.InvalidDatagramsAreDropped]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.InvalidDatagramsAreDropped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.InvalidDatagramsAreDropped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:342]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.PutIsStoredOnPrimaryAndReplica]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.PutIsStoredOnPrimaryAndReplica]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.PutIsStoredOnPrimaryAndReplica]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:351]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.DeleteRemovesBothCopies]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.DeleteRemovesBothCopies]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.DeleteRemovesBothCopies]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:363]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.ReadSurvivesDeadPrimary]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.ReadSurvivesDeadPrimary]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.ReadSurvivesDeadPrimary]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:373]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.KnownDeadPrimaryIsSkippedForReads]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.KnownDeadPrimaryIsSkippedForReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.KnownDeadPrimaryIsSkippedForReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:389]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.ReplicaAnswersFromItsOwnStoreWhenPrimaryKnownDown]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.ReplicaAnswersFromItsOwnStoreWhenPrimaryKnownDown]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.ReplicaAnswersFromItsOwnStoreWhenPrimaryKnownDown]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:403]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.WriteToKnownDeadPrimaryFailsFast]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.WriteToKnownDeadPrimaryFailsFast]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.WriteToKnownDeadPrimaryFailsFast]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:417]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.DisconnectedReplicaGivesReplicationFailed]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.DisconnectedReplicaGivesReplicationFailed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.DisconnectedReplicaGivesReplicationFailed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:428]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.SilentReplicaGivesReplicationFailedAfterTimeout]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.SilentReplicaGivesReplicationFailedAfterTimeout]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.SilentReplicaGivesReplicationFailedAfterTimeout]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:438]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.ReplicaRejectsKeysItIsNotResponsibleFor]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.ReplicaRejectsKeysItIsNotResponsibleFor]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.ReplicaRejectsKeysItIsNotResponsibleFor]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:454]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.RetriedDeleteGetsTheSameAnswer]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.RetriedDeleteGetsTheSameAnswer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.RetriedDeleteGetsTheSameAnswer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:470]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ClusterTest.RetryWhileReplicatingIsNotExecutedTwice]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ClusterTest.RetryWhileReplicatingIsNotExecutedTwice]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ClusterTest.RetryWhileReplicatingIsNotExecutedTwice]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/router_test.cpp:492]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.HandshakeEstablishesBothSides]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.HandshakeEstablishesBothSides]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.HandshakeEstablishesBothSides]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:56]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.MessagesAfterHandshakeAreDelivered]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.MessagesAfterHandshakeAreDelivered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.MessagesAfterHandshakeAreDelivered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:75]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.HelloArrivingByteByByteStillWorks]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.HelloArrivingByteByByteStillWorks]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.HelloArrivingByteByByteStillWorks]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:90]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.CannotSendBeforeHandshake]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.CannotSendBeforeHandshake]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.CannotSendBeforeHandshake]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:106]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.UnknownPeerIsRejected]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.UnknownPeerIsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.UnknownPeerIsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:115]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.PeerClaimingOurOwnIdIsRejected]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.PeerClaimingOurOwnIdIsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.PeerClaimingOurOwnIdIsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:127]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.FirstMessageMustBeHello]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.FirstMessageMustBeHello]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.FirstMessageMustBeHello]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:135]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.WrongNodeAnsweringIsRejected]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.WrongNodeAnsweringIsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.WrongNodeAnsweringIsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:143]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.GarbageClosesTheConnection]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.GarbageClosesTheConnection]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.GarbageClosesTheConnection]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:152]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.SecondHelloIsAProtocolError]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.SecondHelloIsAProtocolError]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.SecondHelloIsAProtocolError]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:160]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PeerSession.NothingIsProcessedAfterClose]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=PeerSession.NothingIsProcessedAfterClose]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PeerSession.NothingIsProcessedAfterClose]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:173]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Heartbeat.PingIsAnsweredWithPongAndResetsTheCount]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Heartbeat.PingIsAnsweredWithPongAndResetsTheCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Heartbeat.PingIsAnsweredWithPongAndResetsTheCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:208]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Heartbeat.PongCarriesThePingsRequestId]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Heartbeat.PongCarriesThePingsRequestId]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Heartbeat.PongCarriesThePingsRequestId]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:222]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Heartbeat.SilentPeerIsDeclaredDead]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Heartbeat.SilentPeerIsDeclaredDead]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Heartbeat.SilentPeerIsDeclaredDead]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:240]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Heartbeat.LatePongStillCountsAsAlive]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Heartbeat.LatePongStillCountsAsAlive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Heartbeat.LatePongStillCountsAsAlive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:252]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Heartbeat.PongForAPingNeverSentIsIgnored]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Heartbeat.PongForAPingNeverSentIsIgnored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Heartbeat.PongForAPingNeverSentIsIgnored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:265]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Heartbeat.NothingIsSentBeforeTheHandshake]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Heartbeat.NothingIsSentBeforeTheHandshake]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Heartbeat.NothingIsSentBeforeTheHandshake]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:272]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Heartbeat.BothSidesCanMonitorEachOther]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=Heartbeat.BothSidesCanMonitorEachOther]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Heartbeat.BothSidesCanMonitorEachOther]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/peer_session_test.cpp:281]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResponseCache.FindsWhatWasInserted]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ResponseCache.FindsWhatWasInserted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResponseCache.FindsWhatWasInserted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/response_cache_test.cpp:12]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResponseCache.SenderAndRequestIdBothMatter]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ResponseCache.SenderAndRequestIdBothMatter]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResponseCache.SenderAndRequestIdBothMatter]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/response_cache_test.cpp:20]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResponseCache.ForgetsTheOldestWhenFull]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ResponseCache.ForgetsTheOldestWhenFull]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResponseCache.ForgetsTheOldestWhenFull]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/response_cache_test.cpp:27]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResponseCache.ReinsertingDoesNotGrowIt]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=ResponseCache.ReinsertingDoesNotGrowIt]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResponseCache.ReinsertingDoesNotGrowIt]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/response_cache_test.cpp:38]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TCPConnection.DeliversData]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=TCPConnection.DeliversData]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TCPConnection.DeliversData]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/tcp_connection_test.cpp:46]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TCPConnection.DataFollowedByFinIsDeliveredThenCloses]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=TCPConnection.DataFollowedByFinIsDeliveredThenCloses]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TCPConnection.DataFollowedByFinIsDeliveredThenCloses]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/tcp_connection_test.cpp:55]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TCPConnection.FinWithoutRdhupFlagIsStillNoticed]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=TCPConnection.FinWithoutRdhupFlagIsStillNoticed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TCPConnection.FinWithoutRdhupFlagIsStillNoticed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/tcp_connection_test.cpp:68]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[StatusServiceTest.StatusDescribesEveryNode]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=StatusServiceTest.StatusDescribesEveryNode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[StatusServiceTest.StatusDescribesEveryNode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/status_service_test.cpp:48]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[StatusServiceTest.LocateReturnsTheReplicaSetPrimaryFirst]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=StatusServiceTest.LocateReturnsTheReplicaSetPrimaryFirst]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[StatusServiceTest.LocateReturnsTheReplicaSetPrimaryFirst]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/status_service_test.cpp:69]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[StatusServiceTest.LocateWithOversizedKeyIsRejected]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=StatusServiceTest.LocateWithOversizedKeyIsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[StatusServiceTest.LocateWithOversizedKeyIsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/status_service_test.cpp:86]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[StatusServiceTest.OtherMessagesAreLeftToTheRouter]=]  /home/av/Documents/projects/cpp/k_v_db/build/unit_tests [==[--gtest_filter=StatusServiceTest.OtherMessagesAreLeftToTheRouter]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[StatusServiceTest.OtherMessagesAreLeftToTheRouter]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/av/Documents/projects/cpp/k_v_db/tests/status_service_test.cpp:95]==]
    WORKING_DIRECTORY [==[/home/av/Documents/projects/cpp/k_v_db/build]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
set(unit_tests_TESTS [==[UniqueFD.DefaultConstructedIsInvalid]==] [==[UniqueFD.WrapsDescriptor]==] [==[UniqueFD.DestructorClosesDescriptor]==] [==[UniqueFD.MoveConstructorTransfersOwnership]==] [==[UniqueFD.MoveAssignmentClosesOldAndTakesNew]==] [==[UniqueFD.SelfMoveAssignmentKeepsDescriptor]==] [==[UniqueFD.ReleaseGivesUpOwnershipWithoutClosing]==] [==[UniqueFD.ResetWithoutArgumentCloses]==] [==[UniqueFD.ResetWithNewDescriptorClosesOld]==] [==[UniqueFD.ResetWithSameDescriptorDoesNotClose]==] [==[ByteIO.WritesBigEndian]==] [==[ByteIO.ReaderNeverReadsPastEnd]==] [==[Codec.HeaderStartsWithMagicAndHasFixedSize]==] [==[Codec.PutRequestRoundTrip]==] [==[Codec.GetResponseWithStatusAndValue]==] [==[Codec.NotFoundResponseHasNoPayload]==] [==[Codec.ForwardedFlagSurvivesRoundTrip]==] [==[Codec.HelloRoundTrip]==] [==[Codec.LargestPutFitsInOneDatagram]==] [==[Codec.EncodingOversizedKeyThrows]==] [==[Codec.TcpWaitsForEveryMissingByte]==] [==[Codec.TcpParsesTwoMessagesFromOneBuffer]==] [==[Codec.RejectsBadMagicImmediately]==] [==[Codec.RejectsWrongVersion]==] [==[Codec.RejectsUnknownType]==] [==[Codec.RejectsInvalidStatus]==] [==[Codec.RejectsHugeLengthFromHeaderAlone]==] [==[Codec.DatagramMustNotBeTruncated]==] [==[Codec.DatagramMustNotHaveTrailingBytes]==] [==[Codec.PayloadWithTrailingBytesIsMalformed]==] [==[Codec.PayloadLengthLargerThanDataIsMalformed]==] [==[Codec.OversizedKeyIsRejectedWhenDecoding]==] [==[Codec.StatusRoundTrip]==] [==[Codec.LocateRoundTrip]==] [==[Codec.LargestStatusFitsInOneDatagram]==] [==[Codec.StatusWithInvalidNodeStateIsMalformed]==] [==[Codec.StatusWithTruncatedNodeListIsMalformed]==] [==[KeyValueStore.GetMissingKeyReturnsNothing]==] [==[KeyValueStore.PutThenGet]==] [==[KeyValueStore.PutOverwritesExistingValue]==] [==[KeyValueStore.RemoveReportsWhetherKeyExisted]==] [==[KeyValueStore.ValuesMayContainAnyBytes]==] [==[RequestHandlerTest.PutThenGetReturnsValue]==] [==[RequestHandlerTest.ResponseCopiesRequestIdentity]==] [==[RequestHandlerTest.GetMissingKeyIsNotFound]==] [==[RequestHandlerTest.PutOverwrites]==] [==[RequestHandlerTest.DeleteRemovesKey]==] [==[RequestHandlerTest.MalformedPayloadIsInvalidRequest]==] [==[RequestHandlerTest.OversizedValueIsReported]==] [==[RequestHandlerTest.NonRequestMessagesAreIgnored]==] [==[Hash.Fnv1aMatchesReferenceValues]==] [==[Hash.IsDeterministicAndSpreads]==] [==[Membership.NodeIdComesFromCanonicalAddress]==] [==[Membership.RejectsDuplicateNodes]==] [==[Membership.ClosestNodeUsesXorDistance]==] [==[Membership.AllNodesAgreeOnOwners]==] [==[Membership.KeysSpreadOverAllNodes]==] [==[Membership.ReplicaSetIsTheClosestNodesInOrder]==] [==[Membership.AllNodesAgreeOnReplicaSets]==] [==[Membership.SmallClusterHasFewerReplicas]==] [==[ClusterTest.RequestToOwnerIsHandledLocally]==] [==[ClusterTest.PutThroughNonOwnerLandsOnOwner]==] [==[ClusterTest.AnyNodeCanReadAnyKey]==] [==[ClusterTest.ForwardedRequestIsNeverForwardedAgain]==] [==[ClusterTest.ForwardedWriteToReplicaIsRefused]==] [==[ClusterTest.ForwardedReadToReplicaIsServed]==] [==[ClusterTest.DeadOwnerGivesUnavailableAfterTimeout]==] [==[ClusterTest.ClientRetryIsNotForwardedTwice]==] [==[ClusterTest.ResponseFromWrongNodeIsIgnored]==] [==[ClusterTest.InvalidDatagramsAreDropped]==] [==[ClusterTest.PutIsStoredOnPrimaryAndReplica]==] [==[ClusterTest.DeleteRemovesBothCopies]==] [==[ClusterTest.ReadSurvivesDeadPrimary]==] [==[ClusterTest.KnownDeadPrimaryIsSkippedForReads]==] [==[ClusterTest.ReplicaAnswersFromItsOwnStoreWhenPrimaryKnownDown]==] [==[ClusterTest.WriteToKnownDeadPrimaryFailsFast]==] [==[ClusterTest.DisconnectedReplicaGivesReplicationFailed]==] [==[ClusterTest.SilentReplicaGivesReplicationFailedAfterTimeout]==] [==[ClusterTest.ReplicaRejectsKeysItIsNotResponsibleFor]==] [==[ClusterTest.RetriedDeleteGetsTheSameAnswer]==] [==[ClusterTest.RetryWhileReplicatingIsNotExecutedTwice]==] [==[PeerSession.HandshakeEstablishesBothSides]==] [==[PeerSession.MessagesAfterHandshakeAreDelivered]==] [==[PeerSession.HelloArrivingByteByByteStillWorks]==] [==[PeerSession.CannotSendBeforeHandshake]==] [==[PeerSession.UnknownPeerIsRejected]==] [==[PeerSession.PeerClaimingOurOwnIdIsRejected]==] [==[PeerSession.FirstMessageMustBeHello]==] [==[PeerSession.WrongNodeAnsweringIsRejected]==] [==[PeerSession.GarbageClosesTheConnection]==] [==[PeerSession.SecondHelloIsAProtocolError]==] [==[PeerSession.NothingIsProcessedAfterClose]==] [==[Heartbeat.PingIsAnsweredWithPongAndResetsTheCount]==] [==[Heartbeat.PongCarriesThePingsRequestId]==] [==[Heartbeat.SilentPeerIsDeclaredDead]==] [==[Heartbeat.LatePongStillCountsAsAlive]==] [==[Heartbeat.PongForAPingNeverSentIsIgnored]==] [==[Heartbeat.NothingIsSentBeforeTheHandshake]==] [==[Heartbeat.BothSidesCanMonitorEachOther]==] [==[ResponseCache.FindsWhatWasInserted]==] [==[ResponseCache.SenderAndRequestIdBothMatter]==] [==[ResponseCache.ForgetsTheOldestWhenFull]==] [==[ResponseCache.ReinsertingDoesNotGrowIt]==] [==[TCPConnection.DeliversData]==] [==[TCPConnection.DataFollowedByFinIsDeliveredThenCloses]==] [==[TCPConnection.FinWithoutRdhupFlagIsStillNoticed]==] [==[StatusServiceTest.StatusDescribesEveryNode]==] [==[StatusServiceTest.LocateReturnsTheReplicaSetPrimaryFirst]==] [==[StatusServiceTest.LocateWithOversizedKeyIsRejected]==] [==[StatusServiceTest.OtherMessagesAreLeftToTheRouter]==])
