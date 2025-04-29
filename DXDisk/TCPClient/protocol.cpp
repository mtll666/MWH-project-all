#include "protocol.h"
#include "UniversalFunction.h"
#include <QDebug>

// 创建共享PDU，确保内存分配安全
std::shared_ptr<PDU> mk_shared_PDU(uint uiMsgLen)
{
    // 计算总协议单元大小
    uint uiPDULen = sizeof(PDU) + uiMsgLen;
    // 使用智能指针管理内存
    std::shared_ptr<PDU> pdu((PDU*)calloc(1, uiPDULen), [](PDU* p) { free(p); });

    if (!pdu) {
        qDebug() << "mk_shared_PDU: Memory allocation failed!";
        exit(EXIT_FAILURE);
    }

    // 初始化PDU字段
    pdu->uiPDULen = uiPDULen;
    pdu->uiMsgLen = uiMsgLen;
    pdu->uiMsgType = 0;
    memset(pdu->caData, 0, sizeof(pdu->caData));
    if (uiMsgLen > 0) {
        memset(pdu->caMsg, 0, uiMsgLen);
    }

    return pdu;
}

// 调试显示PDU信息
void showPDUInfo(std::shared_ptr<PDU> pdu)
{
    if (!pdu) {
        qDebug() << "showPDUInfo: Null PDU!";
        return;
    }

    qDebug() << "PDU INFO BEGIN:";
    qDebug() << "PDULen:" << pdu->uiPDULen;
    qDebug() << "MsgType:" << pdu->uiMsgType;
    qDebug() << "Data:" << extractValidString(pdu->caData, sizeof(pdu->caData));
    qDebug() << "MsgLen:" << pdu->uiMsgLen;
    qDebug() << "Msg:" << extractValidString(pdu->caMsg, pdu->uiMsgLen);
    qDebug() << "PDU INFO END:";
}
