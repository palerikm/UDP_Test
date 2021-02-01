
#include <stdlib.h>
#include <doctest/doctest.h>
#include <fivetuple.h>

TEST_CASE("testing the fivetuple alike function") {
    struct sockaddr_storage src1, dst1;
    sockaddr_initFromString( (struct sockaddr*)&src1,
                             "192.168.1.2" );
    sockaddr_initFromString( (struct sockaddr*)&dst1,
                             "158.38.48.10" );

    struct FiveTuple *a = makeFiveTuple((struct sockaddr*)&src1, (struct sockaddr*)&src1, 5004);
    struct FiveTuple *b = makeFiveTuple((struct sockaddr*)&src1, (struct sockaddr*)&src1, 5004);
    CHECK(fiveTupleAlike(a,b));
    free(a);
    free(b);

    a = makeFiveTuple((struct sockaddr*)&src1, (struct sockaddr*)&src1, 5004);
    b = makeFiveTuple((struct sockaddr*)&src1, (struct sockaddr*)&src1, 5009);
    CHECK(fiveTupleAlike(a,b) == false);
    free(a);
    free(b);

    a = makeFiveTuple((struct sockaddr*)&src1, (struct sockaddr*)&dst1, 5004);
    b = makeFiveTuple((struct sockaddr*)&src1, (struct sockaddr*)&src1, 5004);
    CHECK(fiveTupleAlike(a,b) == false);
    free(a);
    free(b);
}
