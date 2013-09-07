#include "Model/cloud_filesystem_tree.h"
#include "Model/cloud_desktop_node.h"
#include "Model/cloud_category_node.h"
#include "Model/cloud_file_node.h"

#include <tr1/functional>
#include "XiaoMi/MiCloudService.h"
#include "XiaoMi/XiaoMiServiceFactory.h"
#include "DownloadManager/IDownloader.h"
#include "Common/FileManager_DK.h"

using namespace xiaomi;

namespace dk {

namespace document_model {

bool CloudFileSystemTree::local_cache_inited_ = false;
CloudNodes CloudFileSystemTree::all_nodes_;
CloudLocalMap CloudFileSystemTree::cloud_local_map_;
int64_t CloudFileSystemTree::total_size_ = -1;
int64_t CloudFileSystemTree::available_ = -1;
int64_t CloudFileSystemTree::ns_used_ = -1;

CloudFileSystemTree::CloudFileSystemTree()
    : root_node_(0)
{
    initialize();

    MiCloudService* cloud_service = XiaoMiServiceFactory::GetMiCloudService();
    SubscribeMessageEvent(
        MiCloudService::EventFileCreated,
            *cloud_service,
            std::tr1::bind(
            std::tr1::mem_fn(&CloudFileSystemTree::onFileCreated),
                this,
                std::tr1::placeholders::_1));

    SubscribeMessageEvent(
        MiCloudService::EventQuotaRetrieved,
            *cloud_service,
            std::tr1::bind(
            std::tr1::mem_fn(&CloudFileSystemTree::onQuotaRetrieved),
                this,
                std::tr1::placeholders::_1));

    SubscribeMessageEvent(
        MiCloudService::EventChildrenRetrieved,
            *cloud_service,
            std::tr1::bind(
            std::tr1::mem_fn(&CloudFileSystemTree::onChildrenRetrieved),
                this,
                std::tr1::placeholders::_1));

    SubscribeMessageEvent(
        MiCloudService::EventDirectoryCreated,
            *cloud_service,
            std::tr1::bind(
            std::tr1::mem_fn(&CloudFileSystemTree::onDirectoryCreated),
                this,
                std::tr1::placeholders::_1));

    SubscribeMessageEvent(
        MiCloudService::EventRequestDownloadFinished,
            *cloud_service,
            std::tr1::bind(
            std::tr1::mem_fn(&CloudFileSystemTree::onRequestDownloadFinished),
                this,
                std::tr1::placeholders::_1));

    SubscribeMessageEvent(
        MiCloudService::EventFileDeleted,
            *cloud_service,
            std::tr1::bind(
            std::tr1::mem_fn(&CloudFileSystemTree::onFileDeleted),
                this,
                std::tr1::placeholders::_1));

    SubscribeMessageEvent(
        MiCloudService::EventDirectoryDeleted,
            *cloud_service,
            std::tr1::bind(
            std::tr1::mem_fn(&CloudFileSystemTree::onDirectoryDeleted),
                this,
                std::tr1::placeholders::_1));

    // connect download manager
    SubscribeMessageEvent(
        IDownloader::EventDownloadProgressUpdate,
        *IDownloader::GetInstance(),
        std::tr1::bind(
            std::tr1::mem_fn(&CloudFileSystemTree::onDownloadProgress),
            this,
            std::tr1::placeholders::_1));

    // connect local file mananger
    // if file list changed, the cache should be updated
    SubscribeMessageEvent(
        CDKFileManager::EventFileListChanged,
        *CDKFileManager::GetFileManager(),
        std::tr1::bind(
            std::tr1::mem_fn(&CloudFileSystemTree::onLocalFileSystemChanged),
            this,
            std::tr1::placeholders::_1));
    cloud_service->GetQuota();
}

CloudFileSystemTree::~CloudFileSystemTree()
{
}

void CloudFileSystemTree::initialize()
{
    setCurrentNode(&root_node_);
}

ContainerNode * CloudFileSystemTree::root()
{
    return &root_node_;
}

ContainerNode * CloudFileSystemTree::currentNode()
{
    return current_node_;
}

NodeType CloudFileSystemTree::currentType()
{
    return current_node_->type();
}

ContainerNode * CloudFileSystemTree::cdRootNode()
{
    return setCurrentNode(&root_node_);
}

ContainerNode * CloudFileSystemTree::cdContainerNodeWithinTopNode(const string& name)
{
    NodePtr ptr = root_node_.node(name);
    if (ptr == 0)
    {
        return 0;
    }
    return setCurrentNode(dynamic_cast<ContainerNode *>(ptr.get()));
}

ContainerNode * CloudFileSystemTree::cd(NodePtr p)
{
    return setCurrentNode(dynamic_cast<ContainerNode *>(p.get()));
}

ContainerNode * CloudFileSystemTree::cd(const string& name)
{
    // Get child node.
    NodePtr ptr = current_node_->node(name);
    if (ptr != 0)
    {
        return setCurrentNode(dynamic_cast<ContainerNode *>(ptr.get()));
    }
    return 0;
}

ContainerNode * CloudFileSystemTree::cdUp()
{
    if (!canGoUp())
    {
        return 0;
    }

    return setCurrentNode(dynamic_cast<ContainerNode *>(current_node_->mutableParent()));
}

bool CloudFileSystemTree::cdPath(const ModelPath& path)
{
    if (path.size() <= 0)
    {
        return false;
    }

    string root_path = path.front();
    if (root_path != root_node_.absolutePath())
    {
        return false;
    }
    Node* node = &root_node_;
    for (int i = 1; i < path.size(); i++)
    {
        if (node == 0)
        {
            return false;
        }
        ContainerNode* container = dynamic_cast<ContainerNode*>(node);
        if (container != 0)
        {
            node = container->node(path[i]).get();
        }
        else
        {
            return false;
        }
    }

    ContainerNode* current_node = dynamic_cast<ContainerNode*>(node);
    if (current_node != 0)
    {
        setCurrentNode(current_node);
        return true;
    }
    return false;
}

bool CloudFileSystemTree::cdPath(const string& path)
{
    // traverse the whole tree to find the node because path here is actually the id of category or book id
    // Not support now
    return false;
}

bool CloudFileSystemTree::canGoUp()
{
    return (current_node_ != &root_node_);
}

Field CloudFileSystemTree::sortField() const
{
    return by_;
}

SortOrder CloudFileSystemTree::sortOrder() const
{
    return order_;
}

void CloudFileSystemTree::setSortCriteria(Field by, SortOrder order)
{
    by_ = by;
    order_ = order;
}

void CloudFileSystemTree::setSortField(Field by)
{
    by_ = by;
}

void CloudFileSystemTree::sort()
{
    // TODO. Update sort results
    current_node_->sort(by_, order_);
}

DKDisplayMode CloudFileSystemTree::displayMode()
{
    return root_node_.currentDisplayMode();
}

void CloudFileSystemTree::setDisplayMode(DKDisplayMode display_mode)
{
    root_node_.mutableDisplayMode() = display_mode;
}

void CloudFileSystemTree::search(const string& keyword)
{
    root_node_.search(keyword);
}

ContainerNode* CloudFileSystemTree::setCurrentNode(ContainerNode *node)
{
    current_node_ = node;
    mapFilePathToModelPath(current_node_, current_path_);
    return current_node_;
}

bool CloudFileSystemTree::mapFilePathToModelPath(Node* node, ModelPath & model_path)
{
    ModelPath reversed_path;
    Node* current_node = node;
    while (current_node != 0)
    {
        reversed_path.push_back(current_node->absolutePath());
        current_node = current_node->mutableParent();
    }
    if (reversed_path.empty())
    {
        return false;
    }

    model_path.clear();
    // reverse
    for (int i = reversed_path.size() - 1; i >= 0; i--)
    {
        model_path.push_back(reversed_path[i]);
    }
    return true;
}

ContainerNode * CloudFileSystemTree::containerNodeWithinTopNode(NodeType type)
{
    ContainerNode * node = dynamic_cast<ContainerNode *>(root_node_.node(type).get());
    return node;
}

bool CloudFileSystemTree::addNodeToGlobalMap(Node* node)
{
    if (node->id().empty())
    {
        return false;
    }
    if (all_nodes_.find(node->id()) != all_nodes_.end())
    {
        return false;
    }
    all_nodes_[node->id()] = node;
    return true;
}

bool CloudFileSystemTree::removeNodeFromGlobalMap(const string& id)
{
    CloudNodes::iterator itr = all_nodes_.find(id);
    if (itr == all_nodes_.end())
    {
        return false;
    }
    all_nodes_.erase(itr);
    return true;
}

Node* CloudFileSystemTree::getNodeFromGlobalMap(const string& id)
{
    CloudNodes::iterator itr = all_nodes_.find(id);
    if (itr == all_nodes_.end())
    {
        return 0;
    }
    return itr->second;
}

bool CloudFileSystemTree::onFileCreated(const EventArgs& args)
{
    const FileCreatedArgs & file_create_args = dynamic_cast<const FileCreatedArgs&>(args);
    if (file_create_args.succeeded)
    {
        MiCloudServiceResultCreateFileSPtr create_info = file_create_args.result;
        MiCloudFileSPtr cloud_file_info = create_info->GetFileInfo();
        if (cloud_file_info == 0)
        {
            // has not been uploaded yet
            return false;
        }
        string parent_id = cloud_file_info->parentId;

        Node* parent_node = CloudFileSystemTree::getNodeFromGlobalMap(parent_id);
        if (parent_node->type() == NODE_TYPE_MICLOUD_DESKTOP)
        {
            CloudDesktopNode* desktop = dynamic_cast<CloudDesktopNode*>(parent_node);
            //desktop->updateChildren(children_info->GetFileList());
            desktop->onFileCreated(args);
        }
        else if (parent_node->type() == NODE_TYPE_MICLOUD_CATEGORY)
        {
            CloudCategoryNode* category = dynamic_cast<CloudCategoryNode*>(parent_node);
            //category->updateChildren(children_info->GetFileList());
            category->onFileCreated(args);
        }
    }
    return true;
}

bool CloudFileSystemTree::onQuotaRetrieved(const EventArgs& args)
{
    const QuotaRetrievedArgs& quota_retrieved_args = dynamic_cast<const QuotaRetrievedArgs&>(args);
    if (quota_retrieved_args.succeeded && quota_retrieved_args.result != 0)
    {
        CloudFileSystemTree::total_size_ = quota_retrieved_args.result->GetTotal();
        CloudFileSystemTree::available_  = quota_retrieved_args.result->GetAvailable();
        CloudFileSystemTree::ns_used_    = quota_retrieved_args.result->GetNsUsed();
    }
}

bool CloudFileSystemTree::onChildrenRetrieved(const EventArgs& args)
{
    const ChildrenRetrievedArgs & children_args = dynamic_cast<const ChildrenRetrievedArgs&>(args);
    if (children_args.succeeded)
    {
        MiCloudServiceResultGetChildrenSPtr children_info = children_args.result;
        string parent_id = children_args.parent_dir_id;
        Node* parent_node = CloudFileSystemTree::getNodeFromGlobalMap(parent_id);
        if (parent_node->type() == NODE_TYPE_MICLOUD_DESKTOP)
        {
            CloudFileSystemTree::local_cache_inited_ = true;
            CloudDesktopNode* desktop = dynamic_cast<CloudDesktopNode*>(parent_node);
            //desktop->updateChildren(children_info->GetFileList());
            desktop->onChildrenReturned(args);
        }
        else if (parent_node->type() == NODE_TYPE_MICLOUD_CATEGORY)
        {
            CloudCategoryNode* category = dynamic_cast<CloudCategoryNode*>(parent_node);
            //category->updateChildren(children_info->GetFileList());
            category->onChildrenReturned(args);
        }
    }
    return true;
}

bool CloudFileSystemTree::onDirectoryCreated(const EventArgs& args)
{
    const DirectoryCreatedArgs & dir_create_args = dynamic_cast<const DirectoryCreatedArgs&>(args);
    if (dir_create_args.succeeded)
    {
        MiCloudServiceResultCreateDirSPtr create_info = dir_create_args.result;
        MiCloudFileSPtr cloud_file_info = create_info->GetDirInfo();
        string parent_id = cloud_file_info->parentId;
        Node* parent_node = CloudFileSystemTree::getNodeFromGlobalMap(parent_id);
        if (parent_node == 0)
        {
            // it should be the root folder of micloud. and this is the callback from creating desktop node
            // skip it
            return true;
        }

        if (parent_node->type() == NODE_TYPE_MICLOUD_DESKTOP)
        {
            CloudDesktopNode* desktop = dynamic_cast<CloudDesktopNode*>(parent_node);
            //desktop->updateChildren(children_info->GetFileList());
            desktop->onDirectoryCreated(args);
        }
        else if (parent_node->type() == NODE_TYPE_MICLOUD_CATEGORY)
        {
            CloudCategoryNode* category = dynamic_cast<CloudCategoryNode*>(parent_node);
            //category->updateChildren(children_info->GetFileList());
            category->onDirectoryCreated(args);
        }
    }
    return true;
}

bool CloudFileSystemTree::onRequestDownloadFinished(const EventArgs& args)
{
    const RequestDownloadFinishedArgs& request_download_args = dynamic_cast<const RequestDownloadFinishedArgs&>(args);
    string file_id = request_download_args.file_id;
    Node* downloading_node = CloudFileSystemTree::getNodeFromGlobalMap(file_id);
    if (downloading_node == 0)
    {
        return false;
    }

    // find the parent of deleted node
    CloudFileNode* downloading_file_node = dynamic_cast<CloudFileNode*>(downloading_node);
    downloading_file_node->onRequestDownloadFinished(args);
    return true;
}

bool CloudFileSystemTree::onDownloadProgress(const EventArgs& args)
{
    const DownloadUpdateEventArgs& download_args = (const DownloadUpdateEventArgs&)args;
    if (download_args.type != IDownloadTask::MICLOUDFILE)
    {
        return false;
    }

    string file_id = download_args.taskId;
    int current_progress = download_args.percentage;
    int state = download_args.state;
    Node* downloading_node = CloudFileSystemTree::getNodeFromGlobalMap(file_id);
    if (downloading_node != 0)
    {
        // find the parent of deleted node
        CloudFileNode* downloading_file_node = dynamic_cast<CloudFileNode*>(downloading_node);
        downloading_file_node->onDownloadingProgress(current_progress, state);
    }
    return true;
}

bool CloudFileSystemTree::onFileDeleted(const EventArgs& args)
{
    const FileDeletedArgs& file_delete_args = dynamic_cast<const FileDeletedArgs&>(args);
    if (file_delete_args.succeeded)
    {
        string delete_file_id = file_delete_args.file_id;
        Node* delete_node = CloudFileSystemTree::getNodeFromGlobalMap(delete_file_id);
        if (delete_node == 0)
        {
            return false;
        }

        // find the parent of deleted node
        CloudFileNode* delete_file_node = dynamic_cast<CloudFileNode*>(delete_node);
        //string parent_id = delete_file_node->fileInfo()->parentId;
        //Node* parent_node = CloudFileSystemTree::getNodeFromGlobalMap(parent_id);
        Node* parent_node = delete_file_node->mutableParent();
        if (parent_node->type() == NODE_TYPE_MICLOUD_DESKTOP)
        {
            CloudDesktopNode* desktop = dynamic_cast<CloudDesktopNode*>(parent_node);
            //desktop->updateChildren(children_info->GetFileList());
            desktop->onFileDeleted(args);
        }
        else if (parent_node->type() == NODE_TYPE_MICLOUD_CATEGORY)
        {
            CloudCategoryNode* category = dynamic_cast<CloudCategoryNode*>(parent_node);
            //category->updateChildren(children_info->GetFileList());
            category->onFileDeleted(args);
        }
    }
    return true;
}

bool CloudFileSystemTree::onDirectoryDeleted(const EventArgs& args)
{
    const DirectoryDeletedArgs& dir_delete_args = dynamic_cast<const DirectoryDeletedArgs&>(args);
    if (dir_delete_args.succeeded)
    {
        string delete_dir_id = dir_delete_args.directory_id;
        Node* delete_node = CloudFileSystemTree::getNodeFromGlobalMap(delete_dir_id);
        if (delete_node == 0)
        {
            return false;
        }

        CloudCategoryNode* delete_dir_node = dynamic_cast<CloudCategoryNode*>(delete_node);
        //string parent_id = delete_dir_node->directoryInfo()->parentId;
        //Node* parent_node = CloudFileSystemTree::getNodeFromGlobalMap(parent_id);
        Node* parent_node = delete_dir_node->mutableParent();
        if (parent_node->type() == NODE_TYPE_MICLOUD_DESKTOP)
        {
            CloudDesktopNode* desktop = dynamic_cast<CloudDesktopNode*>(parent_node);
            desktop->onDirectoryDeleted(args);
        }
        else if (parent_node->type() == NODE_TYPE_MICLOUD_CATEGORY)
        {
            CloudCategoryNode* category = dynamic_cast<CloudCategoryNode*>(parent_node);
            category->onDirectoryDeleted(args);
        }
    }
    return true;
}

bool CloudFileSystemTree::isCloudFileInLocalStorage(const string& cloud_id,
                                                    const string& file_name,
                                                    const int64_t size,
                                                    PCDKFile& local_file)
{
    local_file = PCDKFile();
    CloudLocalMap::iterator itr = cloud_local_map_.find(cloud_id);
    if (itr != cloud_local_map_.end())
    {
        local_file = itr->second;
        return local_file != 0;
    }

    // find the file by name and size
    PCDKFile found_file = CDKFileManager::GetFileManager()->GetFileByName(file_name, size);
    if (found_file != 0)
    {
        local_file = found_file;
        cloud_local_map_[cloud_id] = local_file;
    }
    return local_file != 0;
}

bool CloudFileSystemTree::isLocalFileInCloud(const string& file_name,
                                             const int64_t size,
                                             MiCloudFileSPtr& cloud_file)
{
    if (cloud_local_map_.empty())
    {
        return false;
    }

    CloudLocalMap::const_iterator it = cloud_local_map_.begin();
    for (; it != cloud_local_map_.end(); it++)
    {
        PCDKFile local_file = it->second;
        string name = PathManager::GetFileName(local_file->GetFilePath());
        if (name == file_name &&
            local_file->GetFileSize() == size) // compare file size
        {
            string cloud_file_id = it->first;
            Node* node = all_nodes_[cloud_file_id];
            CloudFileNode* file_node = dynamic_cast<CloudFileNode*>(node);
            cloud_file = file_node->fileInfo();
            return true;
        }
    }
    return false;
}

// local file system
bool CloudFileSystemTree::onLocalFileSystemChanged(const EventArgs& args)
{
    // clear mapping between local and cloud
    //CloudFileSystemTree::cloud_local_map_.clear();

    // update all posterities
    root_node_.updateChildrenInfo();
    return true;
}

}

}
