#ifndef REFUNDDIALOG_H
#define REFUNDDIALOG_H

#include "walletmodel.h"
#include <QDialog>

namespace Ui {
class RefundDialog;
}
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class RefundDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit RefundDialog(const QModelIndex& idx, QWidget *parent = 0);
    ~RefundDialog();
    
    void setModel(WalletModel *model);

private slots:
    void on_buttonBox_accepted();

private:
    const QModelIndex& modelIndex;
    Ui::RefundDialog *ui;
    WalletModel *model;
};

#endif // REFUNDDIALOG_H
