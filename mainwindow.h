#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override; // Override closeEvent to save the job list
private slots:
    void onAddJob();
    void onDeleteJob(); // Slot for deleting a job
    void onJobSelected(QTreeWidgetItem *item, int column);
	void onJobSettingsTriggered();
private:
    QString m_curfilePath;
    Ui::MainWindow *ui;
    QString lastUsedDirectory; // Store the last used directory
    QImage m_curImage;
    QString m_guid;
    QString selectedJobFullPath; // Store the selected job's FullPath
    void loadJobList(); // Load the job list from a file
    void saveJobList(); // Save the job list to a file
    void setupJobDetails();
    void onSendJob();
    bool sendJobProcess(QImage& image, QString& guid);
//    QString generateUniquePrtFilePath(const QString& outputFolder, const QString& imageName);

};
#endif // MAINWINDOW_H#ifndef MAINWINDOW_H
//#define MAINWINDOW_H

