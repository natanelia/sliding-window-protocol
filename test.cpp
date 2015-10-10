#include <stdio.h>
#include "receiverFrame.h"
#include "transmitterFrame.h"

using namespace std;

int main() {
    TransmitterFrame tf(5);
    tf.setData("abc", 3);
    printf("Transmitter Frame\t: "); tf.printBytes(); printf("\n");
    TransmitterFrame tf2(tf.toBytes());
    if (tf2.isError()) return 1;
    tf2.setFrameNumber(tf2.getFrameNumber() + 1);
    printf("Transmitter Frame 2\t: "); tf2.printBytes(); printf("\n");

    ReceiverFrame rf(3);
    rf.setAck(NAK);
    printf("Receiver Frame\t\t: "); rf.printBytes(); printf("\n");
    ReceiverFrame rf2(rf.toBytes());
    rf2.setFrameNumber(rf2.getFrameNumber() + 1);
    printf("Receiver Frame 2\t: "); rf2.printBytes(); printf("\n");
    return 0;
}