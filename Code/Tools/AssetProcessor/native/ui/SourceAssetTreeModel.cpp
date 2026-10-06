/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SourceAssetTreeModel.h"
#include "SourceAssetTreeItemData.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Gem/GemInfo.h>
#include <native/utilities/assetUtils.h>
#include <native/utilities/IPathConversion.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AssetProcessor
{
    AZ_CVAR(bool, ap_disableAssetTreeView, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Disable asset tree for automated tests.");

    SourceAssetTreeModel::SourceAssetTreeModel(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> sharedDbConnection, QObject* parent) :
        AssetTreeModel(sharedDbConnection, parent)
    {
    }

    SourceAssetTreeModel::~SourceAssetTreeModel()
    {
    }

    void SourceAssetTreeModel::ResetModel()
    {
        // Defaults to the legacy view (relative to the engine root) unless the user opts in to the
        // grouped root view through the Settings tab.
        m_groupSourceAssetRoots = AssetUtilities::GetUserSetting<bool>(AssetUtilities::GroupSourceAssetRootsOptionName, false);

        // We need m_root to contain SourceAssetTreeItemData to show the stat column
        m_root.reset(new AssetTreeItem(
            AZStd::make_shared<SourceAssetTreeItemData>(nullptr, nullptr, "", "", true, AzToolsFramework::AssetDatabase::InvalidEntryId),
            m_errorIcon,
            m_folderIcon,
            m_fileIcon));

        if (ap_disableAssetTreeView)
        {
            return;
        }

        m_sourceToTreeItem.clear();
        m_sourceIdToTreeItem.clear();

        // Load stat table and attach matching stat to the source asset
        AZStd::unordered_map<AZStd::string, AZ::s64> statsTable;
        AZStd::string queryString{ "CreateJobs,%" };
        m_sharedDbConnection->QueryStatLikeStatName(
            queryString.c_str(),
            [&](AzToolsFramework::AssetDatabase::StatDatabaseEntry& stat)
            {
                static constexpr int numTokensExpected = 3;
                AZStd::vector<AZStd::string> tokens;
                AZ::StringFunc::Tokenize(stat.m_statName, tokens, ',');
                if (tokens.size() == numTokensExpected)
                {
                    statsTable[tokens[1]] += stat.m_statValue;
                }
                else
                {
                    AZ_Warning(
                        "AssetProcessor",
                        false,
                        "Analysis Job (CreateJob) stat entry \"%s\" could not be parsed and will not be used. Expected %d tokens, but found %d. A wrong "
                        "stat name may be used in Asset Processor code, or the asset database may be corrupted. If you keep encountering "
                        "this warning, report an issue on GitHub with O3DE version number.",
                        stat.m_statName.c_str(),
                        numTokensExpected,
                        tokens.size()); 
                }
                return true;
            });
 
        if (!m_intermediateAssets)
        {
            // AddOrUpdateEntry will remove intermediate assets if they shouldn't be included in this tree.
            m_sharedDbConnection->QuerySourceAndScanfolder(
                [&](AzToolsFramework::AssetDatabase::SourceAndScanFolderDatabaseEntry& sourceAndScanFolder)
                {
                    if (statsTable.count(sourceAndScanFolder.m_sourceName))
                    {
                        AddOrUpdateEntry(sourceAndScanFolder, sourceAndScanFolder, true, statsTable[sourceAndScanFolder.m_sourceName]);
                    }
                    else
                    {
                        AddOrUpdateEntry(sourceAndScanFolder, sourceAndScanFolder, true);
                    }
                    return true; // return true to continue iterating over additional results, we are populating a container
                });
        }
        else
        {
            AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolderEntry;

            IPathConversion* pathConversion = AZ::Interface<IPathConversion>::Get();

            if (!pathConversion || pathConversion->GetIntermediateAssetScanFolderId() == AzToolsFramework::AssetDatabase::InvalidEntryId)
            {
                // If, for some reason, the path conversion interface is not available, then try to retrieve intermediate folder information a different way.
                m_sharedDbConnection->QueryScanFolderByPortableKey(
                    AssetProcessor::IntermediateAssetsFolderName,
                    [&](AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& scanFolder)
                    {
                        scanFolderEntry = scanFolder;
                        return false;
                    });
            }
            else
            {
                m_sharedDbConnection->QueryScanFolderByScanFolderID(
                    pathConversion->GetIntermediateAssetScanFolderId(),
                    [&](AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& scanFolder)
                    {
                        scanFolderEntry = scanFolder;
                        return false;
                    });
            }

            m_sharedDbConnection->QuerySourceByScanFolderID(
                scanFolderEntry.m_scanFolderID,
                [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
                {
                    if (statsTable.count(sourceEntry.m_sourceName))
                    {
                        AddOrUpdateEntry(sourceEntry, scanFolderEntry, true, statsTable[sourceEntry.m_sourceName]);
                    }
                    else
                    {
                        AddOrUpdateEntry(sourceEntry, scanFolderEntry, true);
                    }
                    return true; // return true to continue iterating over additional results, we are populating a container
                });
        }
    }

    void SourceAssetTreeModel::EnsureSourceRootsInitialized()
    {
        if (!m_assetRootSet)
        {
            m_assetRootSet = AssetUtilities::ComputeAssetRoot(m_assetRoot, nullptr);
        }

        if (m_sourceRootsInitialized)
        {
            return;
        }
        m_sourceRootsInitialized = true;

        m_projectRoot = AZ::IO::Path(AZ::Utils::GetProjectPath()).LexicallyNormal();

        AZ::IO::Path engineRoot;
        if (m_assetRootSet)
        {
            engineRoot = AZ::IO::Path(m_assetRoot.absolutePath().toUtf8().constData()).LexicallyNormal();
        }

        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            return;
        }

        AZStd::vector<AzFramework::GemInfo> gemInfoList;
        if (!AzFramework::GetGemsInfo(gemInfoList, *settingsRegistry))
        {
            return;
        }

        for (const AzFramework::GemInfo& gemInfo : gemInfoList)
        {
            for (const AZ::IO::Path& gemSourcePath : gemInfo.m_absoluteSourcePaths)
            {
                const AZ::IO::Path normalizedGemSourcePath = gemSourcePath.LexicallyNormal();

                // Engine and project gems are already grouped by the engine root and project root respectively.
                if ((!engineRoot.empty() && normalizedGemSourcePath.IsRelativeTo(engineRoot)) ||
                    (!m_projectRoot.empty() && normalizedGemSourcePath.IsRelativeTo(m_projectRoot)))
                {
                    continue;
                }
                m_externalGemSourcePaths.push_back(normalizedGemSourcePath);
            }
        }
    }

    AZ::IO::Path SourceAssetTreeModel::ComputeDisplayPath(
        const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& scanFolder, const AZStd::string& sourceName)
    {
        if (!m_assetRootSet)
        {
            m_assetRootSet = AssetUtilities::ComputeAssetRoot(m_assetRoot, nullptr);
        }

        const AZ::IO::Path absoluteSourcePath = (AZ::IO::Path(scanFolder.m_scanFolder) / sourceName).LexicallyNormal();

        const AZ::IO::Path engineRoot =
            m_assetRootSet ? AZ::IO::Path(m_assetRoot.absolutePath().toUtf8().constData()).LexicallyNormal() : AZ::IO::Path();

        if (!m_groupSourceAssetRoots)
        {
            // Legacy behavior: paths are relative to the engine root. When the project or its gems
            // live outside of the engine install this produces a chain of ".." folders.
            if (m_assetRootSet)
            {
                return absoluteSourcePath.LexicallyProximate(engineRoot);
            }
            return absoluteSourcePath;
        }

        EnsureSourceRootsInitialized();

        // Prefixes the path relative to containerPath with the container's folder name, so that a
        // single grouped root (e.g. the project name or the gems directory) is shown at the top level.
        auto GroupPath = [](const AZ::IO::Path& containerPath, const AZ::IO::Path& sourcePath)
        {
            AZ::IO::Path groupedPath(containerPath.Filename());
            groupedPath /= sourcePath.LexicallyProximate(containerPath);
            return groupedPath;
        };

        // It's common for Open 3D Engine game projects and scan folders to be located outside of the
        // engine root. Rather than producing a chain of ".." folders that lead out of the engine root,
        // project sources are grouped under a single folder named after the project.
        if (!m_projectRoot.empty() && absoluteSourcePath != m_projectRoot && absoluteSourcePath.IsRelativeTo(m_projectRoot))
        {
            return GroupPath(m_projectRoot, absoluteSourcePath);
        }

        // Engine sources keep their existing layout, relative to the engine root.
        if (!engineRoot.empty() && absoluteSourcePath != engineRoot && absoluteSourcePath.IsRelativeTo(engineRoot))
        {
            return absoluteSourcePath.LexicallyProximate(engineRoot);
        }

        // External gems are grouped under a single folder named after the directory containing the gem.
        for (const AZ::IO::Path& gemSourcePath : m_externalGemSourcePaths)
        {
            if (absoluteSourcePath != gemSourcePath && absoluteSourcePath.IsRelativeTo(gemSourcePath))
            {
                const AZ::IO::Path gemContainerPath = gemSourcePath.ParentPath();
                if (!gemContainerPath.empty())
                {
                    return GroupPath(gemContainerPath, absoluteSourcePath);
                }
            }
        }

        // Fall back to anchoring at the scan folder itself so that no ".." navigation is produced.
        const AZ::IO::Path scanFolderContainerPath = AZ::IO::Path(scanFolder.m_scanFolder).LexicallyNormal().ParentPath();
        if (!scanFolderContainerPath.empty() && absoluteSourcePath.IsRelativeTo(scanFolderContainerPath))
        {
            return GroupPath(scanFolderContainerPath, absoluteSourcePath);
        }

        if (m_assetRootSet)
        {
            return absoluteSourcePath.LexicallyProximate(engineRoot);
        }

        return absoluteSourcePath;
    }

    void SourceAssetTreeModel::AddOrUpdateEntry(
        const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source,
        const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& scanFolder,
        bool modelIsResetting, AZ::s64 createJobDuration)
    {
        const auto& existingEntry = m_sourceToTreeItem.find(SourceAndScanID(source.m_sourceName, scanFolder.m_scanFolderID));
        if (existingEntry != m_sourceToTreeItem.end())
        {
            AZStd::shared_ptr<SourceAssetTreeItemData> sourceItemData = AZStd::rtti_pointer_cast<SourceAssetTreeItemData>(existingEntry->second->GetData());

            // This item already exists, refresh the related data.
            sourceItemData->m_scanFolderInfo = scanFolder;
            sourceItemData->m_sourceInfo = source;
            if (createJobDuration != -1)
            {
                // existing item: update duration only if it is provided
                sourceItemData->m_analysisDuration = createJobDuration;
            }

            QModelIndex existingIndexStart = createIndex(existingEntry->second->GetRow(), 0, existingEntry->second);
            QModelIndex existingIndexEnd = createIndex(existingEntry->second->GetRow(), existingEntry->second->GetColumnCount() - 1, existingEntry->second);
            dataChanged(existingIndexStart, existingIndexEnd);
            return;
        }
        AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolderEntry;

        IPathConversion* pathConversion = AZ::Interface<IPathConversion>::Get();
        AZ::s64 intermediateAssetScanFolder = AzToolsFramework::AssetDatabase::InvalidEntryId;
        if(pathConversion)
        {
            intermediateAssetScanFolder = pathConversion->GetIntermediateAssetScanFolderId();
            
            if (intermediateAssetScanFolder != AzToolsFramework::AssetDatabase::InvalidEntryId)
            {
                if (((source.m_scanFolderPK == intermediateAssetScanFolder && !m_intermediateAssets) ||
                     (source.m_scanFolderPK != intermediateAssetScanFolder && m_intermediateAssets)))
                {
                    return;
                }
            }
        }


        AZ::IO::Path fullPath = ComputeDisplayPath(scanFolder, source.m_sourceName);

        if (fullPath.empty())
        {
            AZ_Warning(
                "AssetProcessor", false, "Source id %s has an invalid name: %s", source.m_sourceGuid.ToString<AZStd::string>().c_str(),
                source.m_sourceName.c_str());
            return;
        }

        AssetTreeItem* parentItem = m_root.get();
        // Use posix path separator for each child item
        AZ::IO::Path currentFullFolderPath(AZ::IO::PosixPathSeparator);
        const AZ::IO::FixedMaxPath filename = fullPath.Filename();
        fullPath.RemoveFilename();
        AZ::IO::FixedMaxPathString currentPath;
        for (auto pathIt = fullPath.begin(); pathIt != fullPath.end(); ++pathIt)
        {
            currentPath = pathIt->FixedMaxPathString();
            currentFullFolderPath /= currentPath;
            AssetTreeItem* nextParent = parentItem->GetChildFolder(currentPath.c_str());
            if (!nextParent)
            {
                if (!modelIsResetting)
                {
                    QModelIndex parentIndex = parentItem == m_root.get() ? QModelIndex() : createIndex(parentItem->GetRow(), 0, parentItem);
                    Q_ASSERT(checkIndex(parentIndex));
                    beginInsertRows(parentIndex, parentItem->getChildCount(), parentItem->getChildCount());
                }
                nextParent = parentItem->CreateChild(AZStd::make_shared<SourceAssetTreeItemData>(
                    nullptr,
                    nullptr,
                    currentFullFolderPath.Native(),
                    currentPath.c_str(),
                    true,
                    scanFolder.m_scanFolderID));
                m_sourceToTreeItem[SourceAndScanID(currentFullFolderPath.Native(), scanFolder.m_scanFolderID)] = nextParent;
                // Folders don't have source IDs, don't add to m_sourceIdToTreeItem
                if (!modelIsResetting)
                {
                    endInsertRows();
                }
            }
            parentItem = nextParent;
        }

        if (!modelIsResetting)
        {
            QModelIndex parentIndex = parentItem == m_root.get() ? QModelIndex() : createIndex(parentItem->GetRow(), 0, parentItem);
            Q_ASSERT(checkIndex(parentIndex));
            beginInsertRows(parentIndex, parentItem->getChildCount(), parentItem->getChildCount());
        }

        m_sourceToTreeItem[SourceAndScanID(source.m_sourceName, scanFolder.m_scanFolderID)] =
            parentItem->CreateChild(AZStd::make_shared<SourceAssetTreeItemData>(
                &source,
                &scanFolder,
                source.m_sourceName,
                AZ::IO::FixedMaxPathString(filename.Native()).c_str(),
                false,
                scanFolder.m_scanFolderID,
                createJobDuration));
        m_sourceIdToTreeItem[source.m_sourceID] = m_sourceToTreeItem[SourceAndScanID(source.m_sourceName, scanFolder.m_scanFolderID)];
        if (!modelIsResetting)
        {
            endInsertRows();
        }
    }

    void SourceAssetTreeModel::OnSourceFileChanged(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
    {
        if (ap_disableAssetTreeView)
        {
            return;
        }

        if (!m_root)
        {
            // we haven't reset the model yet, which means all of this will happen when we do.
            return;
        }

        // Model changes need to be run on the main thread.
        AZ::SystemTickBus::QueueFunction([&, entry]()
            {
                m_sharedDbConnection->QueryScanFolderBySourceID(entry.m_sourceID,
                    [&, entry](AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& scanFolder)
                    {
                        AddOrUpdateEntry(entry, scanFolder, false);
                        return true;
                    });
            });
    }

    void SourceAssetTreeModel::RemoveFoldersIfEmpty(AssetTreeItem* itemToCheck)
    {
        // Don't attempt to remove invalid items, non-folders, folders that still have items in them, or the root.
        if (!itemToCheck || !itemToCheck->GetData()->m_isFolder || itemToCheck->getChildCount() > 0 || !itemToCheck->GetParent())
        {
            return;
        }
        RemoveAssetTreeItem(itemToCheck);
    }

    void SourceAssetTreeModel::RemoveAssetTreeItem(AssetTreeItem* assetToRemove)
    {
        if (!assetToRemove)
        {
            return;
        }

        AssetTreeItem* parent = assetToRemove->GetParent();
        if (!parent)
        {
            return;
        }

        QModelIndex parentIndex = parent == m_root.get() ? QModelIndex() : createIndex(parent->GetRow(), 0, parent);
        Q_ASSERT(checkIndex(parentIndex));

        beginRemoveRows(parentIndex, assetToRemove->GetRow(), assetToRemove->GetRow());

        m_sourceToTreeItem.erase(SourceAndScanID(assetToRemove->GetData()->m_assetDbName, assetToRemove->GetData()->m_scanFolderID));
        const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData = AZStd::rtti_pointer_cast<const SourceAssetTreeItemData>(assetToRemove->GetData());
        if (sourceItemData && sourceItemData->m_hasDatabaseInfo)
        {
            m_sourceIdToTreeItem.erase(sourceItemData->m_sourceInfo.m_sourceID);
        }
        parent->EraseChild(assetToRemove);

        endRemoveRows();

        RemoveFoldersIfEmpty(parent);
    }

    void SourceAssetTreeModel::OnSourceFileRemoved(AZ::s64 sourceId)
    {
        if (ap_disableAssetTreeView)
        {
            return;
        }

        if (!m_root)
        {
            // we haven't reset the model yet, which means all of this will happen when we do.
            return;
        }

        // UI changes need to be done on the main thread.
        AZ::SystemTickBus::QueueFunction([&, sourceId]()
            {
                auto existingSource = m_sourceIdToTreeItem.find(sourceId);
                if (existingSource == m_sourceIdToTreeItem.end() || !existingSource->second)
                {
                    // If the asset being removed wasn't previously cached, then something has gone wrong. Reset the model.
                    Reset();
                    return;
                }
                RemoveAssetTreeItem(existingSource->second);
            });
    }

    QVariant SourceAssetTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return QVariant();
        }
        if (section < 0 || section >= static_cast<int>(SourceAssetTreeColumns::Max))
        {
            return QVariant();
        }

        switch (section)
        {
        case aznumeric_cast<int>(SourceAssetTreeColumns::AnalysisJobDuration):
            return tr("Last Analysis Job Duration");
        default:
            return AssetTreeModel::headerData(section, orientation, role);
        }

    }

    QModelIndex SourceAssetTreeModel::GetIndexForSource(const AZStd::string& source, AZ::s64 scanFolderID)
    {
        if (ap_disableAssetTreeView)
        {
            return QModelIndex();
        }

        auto sourceItem = m_sourceToTreeItem.find(SourceAndScanID(source, scanFolderID));
        if (sourceItem == m_sourceToTreeItem.end())
        {
            return QModelIndex();
        }
        return createIndex(sourceItem->second->GetRow(), 0, sourceItem->second);
    }

    void SourceAssetTreeModel::OnCreateJobsDurationChanged(QString sourceName, AZ::s64 scanFolderID)
    {
        // update the source asset's CreateJob duration, if such asset exists in the tree
        const auto& existingEntry = m_sourceToTreeItem.find(SourceAndScanID(sourceName.toUtf8().constData(), scanFolderID));
        if (existingEntry != m_sourceToTreeItem.end())
        {
            AZStd::shared_ptr<SourceAssetTreeItemData> sourceItemData =
                AZStd::rtti_pointer_cast<SourceAssetTreeItemData>(existingEntry->second->GetData());

            AZ::s64 accumulateJobDuration = 0;
            QString statKey = QString("CreateJobs,%1").arg(sourceName).append("%");
            m_sharedDbConnection->QueryStatLikeStatName(
                statKey.toUtf8().data(),
                [&](AzToolsFramework::AssetDatabase::StatDatabaseEntry statEntry)
                {
                    accumulateJobDuration += statEntry.m_statValue;
                    return true;
                });
            sourceItemData->m_analysisDuration = accumulateJobDuration;

            QModelIndex existingIndex = createIndex(
                existingEntry->second->GetRow(), aznumeric_cast<int>(SourceAssetTreeColumns::AnalysisJobDuration), existingEntry->second);
            dataChanged(existingIndex, existingIndex);
        }
    }
}
