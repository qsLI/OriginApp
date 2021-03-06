#ifndef DUOKAN_CLOUD_DESKTOP_NODE_H_
#define DUOKAN_CLOUD_DESKTOP_NODE_H_

#include "Model/container_node.h"
#include "XiaoMi/MiCloudServiceResult.h"

using namespace xiaomi;

namespace dk {

namespace document_model {

/// Desktop node represents a virtual container contains all nodes.
/// It could be a directory or a category in cms library.
/// The branch node implements a directory based container.
class CloudDesktopNode :  public ContainerNode
{
public:
    CloudDesktopNode(Node * parent = 0);
    virtual ~CloudDesktopNode();

public:
    virtual NodePtrs updateChildrenInfo();

    DKDisplayMode currentDisplayMode() const { return current_display_mode_; }
    DKDisplayMode& mutableDisplayMode() { return current_display_mode_; }

    void updateChildren(const MiCloudFileSPtrList& cloud_file_list);

    // CRUD
    virtual bool remove(bool delete_local_files_if_possible, bool exec_now = false);
    virtual void download(bool exec_now = false);
    virtual void createFile(const string& file_path);
    virtual void createDirectory(const string& dir_name);
    virtual void deleteFile(const string& file_id);
    virtual void deleteDirectory(const string& dir_id);

    void onChildrenReturned(const EventArgs& args);
    void onFileCreated(const EventArgs& args);
    void onDirectoryCreated(const EventArgs& args);
    void onFileDeleted(const EventArgs& args);
    void onDirectoryDeleted(const EventArgs& args);

    void fetchRootID();

protected:
    virtual bool updateChildren();

private:
    virtual void scan(const string& dir, NodePtrs &result);

private:
    // Events Slots
    bool onCloudRootCreated(const EventArgs& args);

private:
    DKDisplayMode current_display_mode_;
    time_t last_update_time_;
};

};  // namespace document_model

};  // namespce dk

#endif  // DUOKAN_CLOUD_DESKTOP_NODE_H_
