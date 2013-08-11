#include "refunddialog.h"
#include "ui_refunddialog.h"
#include "transactiontablemodel.h"
#include "bignum.h"

#include <QModelIndex>
#include <QMessageBox>

RefundDialog::RefundDialog(const QModelIndex& idx, QWidget *parent) :
    QDialog(parent),
    modelIndex(idx),
    ui(new Ui::RefundDialog),
    model(0)
{
    ui->setupUi(this);
}

RefundDialog::~RefundDialog()
{
    delete ui;
}

void RefundDialog::setModel(WalletModel *model)
{
    this->model = model;
}

void RefundDialog::on_buttonBox_accepted()
{
    QString txId = this->modelIndex.data(TransactionTableModel::TxIDRole).toString();

    uint256 hashTx;
    hashTx.SetHex(txId.toStdString());

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    if (!model->refundTransaction(hashTx))
    {
        // Refund failed for unknown reason.
        QMessageBox msgBox(this);
        msgBox.setText("Creating return transaction failed.");
        msgBox.exec();
    }
}
