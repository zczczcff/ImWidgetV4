#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <uectti/type_name.hpp>
#include <cstddef>
#include <type_traits>
#include <stdexcept>
#include <algorithm>
#include <map>
#include <chrono>
#include <iomanip>
#include <list>
#include <initializer_list>
#include <sstream>

#ifdef GetClassName
#undef GetClassName
#endif

namespace ROP
{
    // Ĭ�ϴ�������ص�
    template<typename StringType>
    struct DefaultErrorCallback
    {
        void operator()(const StringType& errorMsg) const
        {
            std::cerr << std::string(errorMsg.begin(), errorMsg.end()) << std::endl;
        }
    };

    // ǰ������
    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        class PropertyObject;

    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        struct PropertyMeta;

    // ����ģ���࣬��װ���Ժ���ö������
    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        class Property
    {
    public:
        using ErrorCallbackType = ErrorCallback;

        // Ĭ�Ϲ��캯�� - ������Ч������
        Property() : m_type(EnumType{}), m_metaPtr(nullptr), m_objPtr(nullptr)
        {
        }

        Property(const EnumType type, const void* metaPtr,
            PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* objPtr)
            : m_type(type), m_metaPtr(metaPtr), m_objPtr(objPtr)
        {
        }

        // �ж������Ƿ���Ч
        bool IsValid() const
        {
            return m_metaPtr != nullptr && m_objPtr != nullptr;
        }

        // ��ȡ����ö������
        EnumType GetType() const
        {
            if (!IsValid())
            {
                ErrorCallback()("Invalid property: cannot get type");
                throw std::runtime_error("Invalid property: cannot get type");
            }
            return m_type;
        }

        // ��ȡ����ֵ��ָ�����ͣ�
        template<typename T>
        T GetValue() const
        {
            if (!IsValid())
            {
                ErrorCallback()("Invalid property: cannot get value");
                throw std::runtime_error("Invalid property: cannot get value");
            }
            if (!m_objPtr)
            {
                ErrorCallback()("Invalid property object");
                throw std::runtime_error("Invalid property object");
            }
            return m_objPtr->template GetPropertyValue<T>(m_metaPtr);
        }

        // ��������ֵ��ָ�����ͣ�
        template<typename T>
        void SetValue(const T& value)
        {
            if (!IsValid())
            {
                ErrorCallback()("Invalid property: cannot set value");
                throw std::runtime_error("Invalid property: cannot set value");
            }
            if (!m_objPtr)
            {
                ErrorCallback()("Invalid property object");
                throw std::runtime_error("Invalid property object");
            }
            m_objPtr->template SetPropertyValue<T>(m_metaPtr, value);
        }

        template<typename T>
        T* GetPointer()
        {
            if (!IsValid())
                return nullptr;
            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(m_metaPtr);
            void* ptr = meta->getter(const_cast<PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(m_objPtr));
            return static_cast<T*>(ptr);
        }

        // ��ȡ����ֵ������
        template<typename T>
        T& GetReference()
        {
            T* ptr = GetPointer<T>();
            if (!ptr)
            {
                ErrorCallback()("Failed to get property reference");
                throw std::runtime_error("Failed to get property reference");
            }
            return *ptr;
        }

        // ��ȡ����ֵ�ĳ�������
        template<typename T>
        const T& GetConstReference() const
        {
            const T* ptr = GetConstPointer<T>();
            if (!ptr)
            {
                ErrorCallback()("Failed to get property const reference");
                throw std::runtime_error("Failed to get property const reference");
            }
            return *ptr;
        }

        template<typename T>
        const T* GetConstPointer() const
        {
            if (!IsValid())
                return nullptr;
            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(m_metaPtr);
            void* ptr = meta->getter(const_cast<PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(m_objPtr));
            return static_cast<const T*>(ptr);
        }

        // ��ȡ����Ԫ����ָ��
        const void* GetMetaPtr() const
        {
            if (!IsValid())
            {
                ErrorCallback()("Invalid property: cannot get meta pointer");
                throw std::runtime_error("Invalid property: cannot get meta pointer");
            }
            return m_metaPtr;
        }

        // ��ȡ��������
        PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* GetObject() const
        {
            if (!IsValid())
            {
                ErrorCallback()("Invalid property: cannot get object");
                throw std::runtime_error("Invalid property: cannot get object");
            }
            return m_objPtr;
        }

        // ��ȡ��������
        StringType GetDescription() const
        {
            if (!IsValid())
                return StringType{};

            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(m_metaPtr);
            return meta ? meta->description : StringType{};
        }

        // ��ȡ�������ƣ�����KeyType��
        KeyType GetName() const
        {
            if (!IsValid())
                return KeyType{};

            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(m_metaPtr);
            return meta ? meta->name : KeyType{};
        }

        // ��ȡ���������ַ�����ʹ��KeyToStringת����
        StringType GetNameString() const
        {
            if (!IsValid())
                return StringType{};

            KeyType name = GetName();
            return KeyToString()(name);
        }

        // ��ȡ������������
        StringType GetClassName() const
        {
            if (!IsValid())
                return StringType{};

            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(m_metaPtr);
            return meta ? meta->className : StringType{};
        }

    private:
        EnumType m_type;
        const void* m_metaPtr;
        PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* m_objPtr;
    };

    // ����Ԫ����
    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        struct PropertyMeta
    {
        KeyType name;
        EnumType enumType;
        StringType typeName;
        size_t offset;
        StringType className;
        std::function<void* (PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*)> getter;
        std::function<void(PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*, void*)> setter;
        bool isCustomAccessor;
        size_t registrationOrder = 0;

        // �������Ƿ�Ϊѡ�����Ա�־
        bool isOptional = false;

        // ��������������
        StringType description;

        bool operator==(const PropertyMeta& other) const
        {
            KeyEqual equal;
            return equal(name, other.name) && className == other.className && enumType == other.enumType;
        }

        bool operator<(const PropertyMeta& other) const
        {
            return registrationOrder < other.registrationOrder;
        }
    };

    // ΪPropertyMeta�ṩ��ϣ֧��
    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        struct PropertyMetaHash
    {
        size_t operator()(const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& prop) const
        {
            KeyHash hash;
            return hash(prop.name) ^
                (std::hash<StringType>()(prop.className) << 1) ^
                (std::hash<int>()(static_cast<int>(prop.enumType)) << 2);
        }
    };

    // ������������
    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        using PropertyMap = std::unordered_map<KeyType,
        PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>, KeyHash, KeyEqual>;

    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        using PropertyMultiMap = std::unordered_multimap<KeyType,
        PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>, KeyHash, KeyEqual>;

    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        using PropertyList = std::vector<PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>>;

    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        using PropertySet = std::unordered_set<
        PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>,
        PropertyMetaHash<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>>;

    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        using ClassNameList = std::vector<StringType>;

    // �������ݽṹ�� - �����о�̬���ݽṹ�ϲ�������
    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        struct PropertyData
    {
        // ����ӳ���
        PropertyMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> ownPropertyMap;          // ��������ӳ���
        PropertyMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> directPropertyMap;       // ֱ������ӳ�����O(1)���ң�
        std::unordered_map<StringType,
            PropertyMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>> parentPropertyMaps; // ��������ӳ��������� -> ���Ա���
        PropertyMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> combinedPropertyMap;     // �ϲ�����������Ա�

        // ���Զ�ӳ������������ͬ�����ԣ�
        PropertyMultiMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> allPropertiesMultiMap;

        // �����б�
        PropertyList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> ownPropertiesList;      // ���������б�����ע��˳��
        PropertyList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> allPropertiesList;      // ���������б��������̳еģ�����ͬ����

        // ������������
        std::vector<KeyType> orderedPropertyNames;                                             // �������԰�ע��˳��������б�
        std::unordered_map<StringType, std::vector<KeyType>> parentOrderedPropertyNames;      // �����������������б�

        // ���������б�ӳ��
        std::unordered_map<StringType,
            PropertyList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>> parentPropertiesListMap;

        // ���������б������̳�˳��ֱ�Ӹ�����ǰ����Զ�����ں�
        ClassNameList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> allParentsName;

        // ������ѡ���б�ӳ�䣨���������������洢��
        std::unordered_map<StringType, std::unordered_map<KeyType, std::vector<StringType>>> optionalPropertyMap;

        // ����������ӳ��������������������洢��
        std::unordered_map<StringType, std::unordered_map<KeyType, StringType>> descriptionMap;

        // ע�������
        size_t registrationCounter = 0;

        // ��ʼ����־
        bool initialized = false;
    };

    // ��ѡ�����࣬�̳���Property���ṩѡ����ع���
    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        class OptionalProperty : public Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>
    {
    public:
        using BasePropertyType = Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>;

        // Ĭ�Ϲ��캯�� - ������Ч�Ŀ�ѡ����
        OptionalProperty() : BasePropertyType()
        {
        }

        OptionalProperty(const BasePropertyType& prop)
            : BasePropertyType(prop)
        {
            // ��PropertyData�л�ȡѡ���б�
            InitializeOptionList();
        }

        OptionalProperty(const OptionalProperty& other)
            : BasePropertyType(other), m_optionList(other.m_optionList)
        {
        }

        OptionalProperty& operator=(const OptionalProperty& other)
        {
            if (this != &other)
            {
                BasePropertyType::operator=(other);
                m_optionList = other.m_optionList;
            }
            return *this;
        }

        // ��ȡ��ǰѡ����ַ���
        StringType GetOptionString() const
        {
            if (!this->IsValid())
                return StringType{};

            // ��ͨ��getter��ȡѡ�������ֵ
            int currentValue = this->template GetValue<int>();

            // �ڵ�ǰ�����������ѡ���б��в���
            auto classOptions = GetOptionListForThisClass();
            if (currentValue >= 0 && currentValue < static_cast<int>(classOptions.size()))
            {
                return classOptions[currentValue];
            }

            // ���û�ҵ���������ѡ���б��в���
            if (currentValue >= 0 && currentValue < static_cast<int>(m_optionList.size()))
            {
                return m_optionList[currentValue];
            }

            return StringType{};
        }

        // ��ȡ����ѡ���б�����ǰ��+���и��ࣩ
        const std::vector<StringType>& GetOptionList() const
        {
            return m_optionList;
        }

        // ͨ���ַ�������ѡ��
        bool SetOptionByString(const StringType& optionStr)
        {
            if (!this->IsValid())
                return false;

            // �ڵ�ǰ�����������ѡ���б��в���
            auto classOptions = GetOptionListForThisClass();
            for (size_t i = 0; i < classOptions.size(); ++i)
            {
                if (classOptions[i] == optionStr)
                {
                    // �ҵ���Ӧ������ͨ��setter��������ֵ
                    this->SetValue<int>(static_cast<int>(i));
                    return true;
                }
            }

            // ���û�ҵ���������ѡ���б��в���
            for (size_t i = 0; i < m_optionList.size(); ++i)
            {
                if (m_optionList[i] == optionStr)
                {
                    // �ҵ���Ӧ������ͨ��setter��������ֵ
                    this->SetValue<int>(static_cast<int>(i));
                    return true;
                }
            }

            return false;
        }

        // ͨ����������ѡ��
        bool SetOptionByIndex(int index)
        {
            if (!this->IsValid())
                return false;

            auto classOptions = GetOptionListForThisClass();
            if (index >= 0 && index < static_cast<int>(classOptions.size()))
            {
                this->SetValue<int>(index);
                return true;
            }

            if (index >= 0 && index < static_cast<int>(m_optionList.size()))
            {
                this->SetValue<int>(index);
                return true;
            }

            return false;
        }

        // ����Ƿ���ѡ������
        bool IsOptional() const
        {
            if (!this->IsValid())
                return false;

            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this->GetMetaPtr());
            return meta && meta->isOptional;
        }

        // ��ȡѡ������
        size_t GetOptionCount() const
        {
            return m_optionList.size();
        }

    private:
        // ��ȡ��ǰ�����������ѡ���б�����PropertyData�в��ң�
        std::vector<StringType> GetOptionListForThisClass() const
        {
            if (!this->IsValid())
                return {};

            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this->GetMetaPtr());
            if (!meta || !meta->isOptional)
                return {};

            auto* obj = this->GetObject();
            if (!obj)
                return {};

            // ͨ��PropertyData��ȡѡ���б�
            auto& propertyData = obj->GetPropertyData();
            auto classIt = propertyData.optionalPropertyMap.find(meta->className);
            if (classIt != propertyData.optionalPropertyMap.end())
            {
                auto propIt = classIt->second.find(meta->name);
                if (propIt != classIt->second.end())
                {
                    return propIt->second;
                }
            }

            return {};
        }

        // ��ʼ��ѡ���б����ϲ���ǰ������и����ѡ�
        void InitializeOptionList()
        {
            m_optionList.clear();

            if (!this->IsValid())
                return;

            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this->GetMetaPtr());
            if (!meta || !meta->isOptional)
                return;

            auto* obj = this->GetObject();
            if (!obj)
                return;

            // ���Ȼ�ȡ��ǰ�����������ѡ���б�
            auto currentClassOptions = GetOptionListForThisClass();
            for (const auto& option : currentClassOptions)
            {
                m_optionList.push_back(option);
            }

            // �����ǰ������Ч������Ҫ��ȡ�����ͬ�����Ե�ѡ���б�
            // ��ȡ���и�������
            const auto& parentNames = obj->GetAllParentsName();

            // ��ÿ�����࣬����ͬ������
            for (const auto& parentClassName : parentNames)
            {
                // ��PropertyData�в��Ҹ����ѡ���б�
                auto& propertyData = obj->GetPropertyData();
                auto parentClassIt = propertyData.optionalPropertyMap.find(parentClassName);
                if (parentClassIt == propertyData.optionalPropertyMap.end())
                    continue;

                auto parentPropIt = parentClassIt->second.find(meta->name);
                if (parentPropIt == parentClassIt->second.end())
                    continue;

                // ���Ӹ����ѡ���Ҫ�����ظ�
                for (const auto& parentOption : parentPropIt->second)
                {
                    // ����Ƿ��Ѵ�����ͬ��ѡ��
                    bool exists = false;
                    for (const auto& existingOption : m_optionList)
                    {
                        if (existingOption == parentOption)
                        {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists)
                    {
                        m_optionList.push_back(parentOption);
                    }
                }
            }
        }

    private:
        std::vector<StringType> m_optionList; // ����ĺϲ����ѡ���б�
    };

    // ==================== �����߼���ȡ - �������� ====================

    // ����ϵͳ������
    template<typename EnumType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        class PropertySystemUtils
    {
    public:
        // ע�Ḹ�����Ե�����ӳ���
        template<typename ParentClass>
        static void RegisterParentProperties(
            PropertyData<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& propertyData,
            const StringType& parentClassName)
        {
            auto& parentPropertyData = ParentClass::GetPropertyDataStatic();

            // ���Ƹ������������
            PropertyMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> parentCopy;
            for (const auto& pair : parentPropertyData.ownPropertyMap)
            {
                parentCopy[pair.first] = pair.second;
            }
            propertyData.parentPropertyMaps[parentClassName] = parentCopy;

            // ���游��������������
            propertyData.parentOrderedPropertyNames[parentClassName] = parentPropertyData.orderedPropertyNames;

            // �ݹ�ע�������������
            for (const auto& parentPair : parentPropertyData.parentPropertyMaps)
            {
                propertyData.parentPropertyMaps[parentPair.first] = parentPair.second;
            }

            // �ݹ�ע���������������������
            for (const auto& parentPair : parentPropertyData.parentOrderedPropertyNames)
            {
                propertyData.parentOrderedPropertyNames[parentPair.first] = parentPair.second;
            }

            // �ݹ�ע���������ѡ������
            for (const auto& parentPair : parentPropertyData.optionalPropertyMap)
            {
                propertyData.optionalPropertyMap[parentPair.first] = parentPair.second;
            }

            // �ݹ�ע�������������
            for (const auto& parentPair : parentPropertyData.descriptionMap)
            {
                propertyData.descriptionMap[parentPair.first] = parentPair.second;
            }
        }

        // �������и��������б�
        template<typename ParentClass>
        static void BuildAllParentsNameList(
            PropertyData<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& propertyData,
            const StringType& parentClassName)
        {
            propertyData.allParentsName.clear();

            if constexpr (!std::is_same_v<ParentClass, PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>>)
            {
                // ����ֱ�Ӹ���
                propertyData.allParentsName.push_back(parentClassName);

                // �ݹ��ȡ����ĸ���
                auto& grandParents = ParentClass::GetPropertyDataStatic().allParentsName;
                for (const auto& grandParent : grandParents)
                {
                    propertyData.allParentsName.push_back(grandParent);
                }
            }
        }

        // �������������б�ӳ��
        static void BuildParentPropertiesListMap(
            PropertyData<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& propertyData)
        {
            propertyData.parentPropertiesListMap.clear();

            // �Ӹ���ӳ�乹�������б�
            for (const auto& classPair : propertyData.parentPropertyMaps)
            {
                const StringType& className = classPair.first;
                const auto& propertyMap = classPair.second;
                auto orderedIt = propertyData.parentOrderedPropertyNames.find(className);

                if (orderedIt != propertyData.parentOrderedPropertyNames.end())
                {
                    // ����ע��˳�򹹽������б�
                    PropertyList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> orderedList;
                    for (const auto& propName : orderedIt->second)
                    {
                        auto propIt = propertyMap.find(propName);
                        if (propIt != propertyMap.end())
                        {
                            orderedList.push_back(propIt->second);
                        }
                    }
                    propertyData.parentPropertiesListMap[className] = orderedList;
                }
            }
        }

        // �������������б����������ࣩ - ����ͬ�����ԣ���������࣬��ÿ����������
        static void BuildAllPropertiesList(
            PropertyData<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& propertyData)
        {
            propertyData.allPropertiesList.clear();
            propertyData.allPropertiesMultiMap.clear();

            // ��ȡ���и������ƣ����̳�˳��ֱ�Ӹ�����ǰ����Զ�����ں�
            auto& allParentsName = propertyData.allParentsName;

            // ��һ�������������Լ������ԣ���ע��˳��
            for (const auto& prop : propertyData.ownPropertiesList)
            {
                propertyData.allPropertiesList.push_back(prop);
                propertyData.allPropertiesMultiMap.insert({ prop.name, prop });
            }

            // �ڶ�����Ȼ�󰴼̳�˳�����Ӹ�������ԣ���ֱ�Ӹ��ൽ��Զ���ȣ�
            for (const auto& parentClassName : allParentsName)
            {
                auto it = propertyData.parentPropertiesListMap.find(parentClassName);
                if (it != propertyData.parentPropertiesListMap.end())
                {
                    for (const auto& prop : it->second)
                    {
                        propertyData.allPropertiesList.push_back(prop);
                        propertyData.allPropertiesMultiMap.insert({ prop.name, prop });
                    }
                }
            }
        }

        // ��ʼ���������ݣ��ϲ�������裩
        static void InitializePropertyData(
            PropertyData<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& propertyData)
        {
            // ��ʼ��ֱ������ӳ��
            propertyData.directPropertyMap.clear();
            for (const auto& propPair : propertyData.ownPropertyMap)
            {
                propertyData.directPropertyMap[propPair.first] = propPair.second;
            }

            // �ϲ��������Ե��������Ա���ͬ������ֻ����һ�����������ࣩ
            propertyData.combinedPropertyMap.clear();

            // ���������и��������
            for (const auto& classPair : propertyData.parentPropertyMaps)
            {
                for (const auto& propPair : classPair.second)
                {
                    // �����û��������ԣ�ͬ������������
                    if (propertyData.combinedPropertyMap.find(propPair.first) == propertyData.combinedPropertyMap.end())
                    {
                        propertyData.combinedPropertyMap[propPair.first] = propPair.second;
                    }
                }
            }

            // Ȼ�������Լ������ԣ��Ḳ�Ǹ����ͬ�����ԣ�
            for (const auto& propPair : propertyData.ownPropertyMap)
            {
                propertyData.combinedPropertyMap[propPair.first] = propPair.second;
            }

            // ����ע��˳���ʼ�����������б�
            propertyData.ownPropertiesList.clear();
            for (const auto& name : propertyData.orderedPropertyNames)
            {
                auto it = propertyData.ownPropertyMap.find(name);
                if (it != propertyData.ownPropertyMap.end())
                {
                    propertyData.ownPropertiesList.push_back(it->second);
                }
            }
        }
    };

    // ����ģ�壬��ö�����Ͳ����ͼ�ֵ���Ͳ���
    template<typename EnumType,
        typename KeyType = std::string,
        typename KeyHash = std::hash<KeyType>,
        typename KeyEqual = std::equal_to<KeyType>,
        typename KeyToString = std::function<std::string(const KeyType&)>,
        typename StringType = std::string,
        typename ErrorCallback = DefaultErrorCallback<StringType>>
        class PropertyObject
    {
        friend class Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>;
    public:
        using ROPEnumClass = EnumType;
        using ROPKeyType = KeyType;
        using ROPKeyHash = KeyHash;
        using ROPKeyEqual = KeyEqual;
        using ROPKeyToString = KeyToString;
        using ROPStringType = StringType;
        using ROPErrorCallback = ErrorCallback;
        using ROPPropertyDataType = PropertyData<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>;
        using ROPObjectType = PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>;
        using ROPProperty = Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>;
        using ROPOptionalProperty = OptionalProperty<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>;

        virtual ~PropertyObject() = default;

        // ��ȡ����
        virtual StringType GetClassName() const = 0;

        // ��ȡ�������ݽṹ�壨��OptionalPropertyʹ�ã�
        virtual const PropertyData<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& GetPropertyData() const = 0;

        static void StaticInitializeProperties() {};

        // ��̬�����������
        static void ReportError(const StringType& errorMsg)
        {
            static ROPErrorCallback s_errorCallback = ROPErrorCallback();
            s_errorCallback(errorMsg);
        }

        // ���ô���ص�����ѡ��
        static void SetErrorCallback(const ROPErrorCallback& callback)
        {
            static ROPErrorCallback s_errorCallback = callback;
            s_errorCallback = callback;
        }

        // ������Ա���� - ͨ��GetPropertyData()ͳһ����

        // ��ȡ�������б�
        const ClassNameList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& GetAllParentsName() const
        {
            return GetPropertyData().allParentsName;
        }

        // ��ȡ������������ԣ��������̳еģ�
        const PropertyList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& GetOwnPropertiesList() const
        {
            return GetPropertyData().ownPropertiesList;
        }

        // ��ȡ�������ԣ������̳еģ��������ͬ�������ԣ�
        const PropertyMultiMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& GetAllPropertiesMultiMap() const
        {
            return GetPropertyData().allPropertiesMultiMap;
        }

        // ��ȡ��������ӳ���
        const std::unordered_map<StringType,
            PropertyMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>>&GetParentPropertiesMap() const
        {
            return GetPropertyData().parentPropertyMaps;
        }

        // ��ȡֱ������ӳ�䣨�������������ԣ�O(1)���ң�
        const PropertyMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& GetDirectPropertyMap() const
        {
            return GetPropertyData().directPropertyMap;
        }

        // ��ȡָ������������б�������
        const PropertyList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& GetParentPropertiesList(const StringType& parentClassName) const
        {
            const auto& parentPropertiesListMap = GetPropertyData().parentPropertiesListMap;
            auto it = parentPropertiesListMap.find(parentClassName);
            if (it != parentPropertiesListMap.end())
            {
                return it->second;
            }
            static PropertyList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> emptyList;
            return emptyList;
        }

        // ��ȡ�������������б��������̳еģ�����ͬ����
        const PropertyList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& GetAllPropertiesList() const
        {
            return GetPropertyData().allPropertiesList;
        }

        // ͨ�����ƻ�ȡ���԰�װ���� - ����ж��ͬ�����ԣ����ص�һ��������ģ�
        Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> GetProperty(const KeyType& name) const
        {
            // ���ȴ�ֱ������ӳ���в��ң�O(1)��
            const auto& directMap = GetDirectPropertyMap();
            auto it = directMap.find(name);
            if (it != directMap.end())
            {
                return Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>(
                    it->second.enumType, &it->second, const_cast<PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this));
            }

            // ���ֱ��ӳ����û�ҵ����ٴ����������в���
            auto& allProps = GetAllPropertiesMultiMap();
            auto range = allProps.equal_range(name);

            if (range.first != range.second)
            {
                return Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>(
                    range.first->second.enumType, &range.first->second, const_cast<PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this));
            }

            // �Ҳ���ʱ������Ч��Property����
            return Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>();
        }

        // ͨ�����ƺ�������ȡ���԰�װ���� - ��ȷ�����ض��������
        Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> GetProperty(const KeyType& name, const StringType& className) const
        {
            auto& allProps = GetAllPropertiesMultiMap();
            auto range = allProps.equal_range(name);

            for (auto it = range.first; it != range.second; ++it)
            {
                if (it->second.className == className)
                {
                    return Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>(
                        it->second.enumType, &it->second, const_cast<PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this));
                }
            }

            // �Ҳ���ʱ������Ч��Property����
            return Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>();
        }

        // ��ȡ����ͬ�����ԣ������б���
        std::vector<Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>> GetAllPropertiesByName(const KeyType& name) const
        {
            std::vector<Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>> result;
            auto& allProps = GetAllPropertiesMultiMap();
            auto range = allProps.equal_range(name);

            for (auto it = range.first; it != range.second; ++it)
            {
                result.push_back(Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>(
                    it->second.enumType, &it->second, const_cast<PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this)));
            }

            return result;
        }

        // ��ȡ�������ԣ������̳еģ�����˳����������࣬ÿ�����ڰ�ע��˳��
        std::vector<Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>> GetAllPropertiesOrdered() const
        {
            std::vector<Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>> result;
            const auto& allPropsList = GetAllPropertiesList();

            for (const auto& meta : allPropsList)
            {
                result.push_back(Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>(
                    meta.enumType, &meta, const_cast<PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this)));
            }

            return result;
        }

    protected:
        // �ڲ�������ͨ������Ԫ����ָ���ȡ����ֵ
        template<typename T>
        T GetPropertyValue(const void* metaPtr) const
        {
            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(metaPtr);
            if (!meta)
            {
                ReportError(StringType("Invalid property meta pointer"));
                throw std::runtime_error("Invalid property meta pointer");
            }

            void* ptr = meta->getter(const_cast<PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this));
            return *reinterpret_cast<T*>(ptr);
        }

        // �ڲ�������ͨ������Ԫ����ָ����������ֵ
        template<typename T>
        void SetPropertyValue(const void* metaPtr, const T& value)
        {
            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(metaPtr);
            if (!meta)
            {
                ReportError(StringType("Invalid property meta pointer"));
                throw std::runtime_error("Invalid property meta pointer");
            }

            T temp = value;
            meta->setter(const_cast<PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this), &temp);
        }

    public:
        // �������ԣ�����������ԣ������̳еģ�
        bool HasProperty(const KeyType& name) const
        {
            // ���ȼ��ֱ������ӳ�䣨O(1)��
            const auto& directMap = GetDirectPropertyMap();
            if (directMap.find(name) != directMap.end())
                return true;

            // �����������
            auto& allProps = GetAllPropertiesMultiMap();
            return allProps.find(name) != allProps.end();
        }

        // ����ض������Ƿ���ָ������
        bool HasProperty(const KeyType& name, const StringType& className) const
        {
            auto& allProps = GetAllPropertiesMultiMap();
            auto range = allProps.equal_range(name);

            for (auto it = range.first; it != range.second; ++it)
            {
                if (it->second.className == className)
                    return true;
            }
            return false;
        }

        // ��ȡָ������������б�������ע��˳��
        PropertyList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> GetParentClassProperties(const StringType& parentClassName) const
        {
            PropertyList<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> result;
            auto& allProps = GetAllPropertiesMultiMap();

            for (const auto& pair : allProps)
            {
                if (pair.second.className == parentClassName)
                {
                    result.push_back(pair.second);
                }
            }

            // ����ע��˳������
            std::sort(result.begin(), result.end(),
                [](const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& a,
                    const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& b)
                {
                    return a.registrationOrder < b.registrationOrder;
                });

            return result;
        }

        // ��ȡָ�����������ӳ���
        PropertyMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> GetParentClassPropertyMap(const StringType& parentClassName) const
        {
            auto& parentMaps = GetParentPropertiesMap();
            auto it = parentMaps.find(parentClassName);
            if (it != parentMaps.end())
            {
                return it->second;
            }
            return PropertyMap<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>();
        }

        // ��Propertyת��ΪOptionalProperty
        OptionalProperty<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> ToOptionalProperty(
            const Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& prop) const
        {
            // ���������Ч��������Ч��OptionalProperty
            if (!prop.IsValid())
            {
                return OptionalProperty<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>();
            }

            const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* meta =
                static_cast<const PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(prop.GetMetaPtr());

            if (!meta || !meta->isOptional)
            {
                ReportError(StringType("Property is not an optional property"));
                throw std::runtime_error("Property is not an optional property");
            }

            return OptionalProperty<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>(prop);
        }

        // ͨ��Property��ȡOptionalProperty
        OptionalProperty<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> GetPropertyAsOptional(const KeyType& name) const
        {
            auto prop = GetProperty(name);
            if (!prop.IsValid())
            {
                return OptionalProperty<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>();
            }
            return ToOptionalProperty(prop);
        }

        // ͨ�����ƺ�������ȡOptionalProperty
        OptionalProperty<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> GetPropertyAsOptional(const KeyType& name, const StringType& className) const
        {
            auto prop = GetProperty(name, className);
            if (!prop.IsValid())
            {
                return OptionalProperty<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>();
            }
            return ToOptionalProperty(prop);
        }

        // ��ȡ���Ե�����
        StringType GetPropertyDescription(const KeyType& name) const
        {
            auto prop = GetProperty(name);
            return prop.GetDescription();
        }

        // ��ȡ���Ժ����������ذ������ƺ��������ַ�����
        StringType GetPropertyWithDescription(const KeyType& name) const
        {
            auto prop = GetProperty(name);
            if (!prop.IsValid())
            {
                return KeyToString()(name) + StringType(" - [Invalid Property]");
            }

            StringType description = prop.GetDescription();
            if (!description.empty())
            {
                return KeyToString()(name) + StringType(" - ") + description;
            }
            return KeyToString()(name);
        }

        // ��ȡ����ͬ�����ԣ���˳����������࣬ÿ�����ڰ�ע��˳��
        std::vector<Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>> GetPropertiesByNameOrdered(const KeyType& name) const
        {
            std::vector<Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>> result;
            const auto& allPropsList = GetAllPropertiesList();

            for (const auto& meta : allPropsList)
            {
                KeyEqual equal;
                if (equal(meta.name, name))
                {
                    result.push_back(Property<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>(
                        meta.enumType, &meta, const_cast<PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*>(this)));
                }
            }

            return result;
        }

        // ��ȡ��������������ͬ�����ԣ�
        size_t GetPropertyCount() const
        {
            return GetAllPropertiesList().size();
        }

        // ��ȡ���ظ������������б�
        std::vector<KeyType> GetUniquePropertyNames() const
        {
            std::unordered_set<KeyType, KeyHash, KeyEqual> uniqueNames;
            const auto& allPropsList = GetAllPropertiesList();

            for (const auto& meta : allPropsList)
            {
                uniqueNames.insert(meta.name);
            }

            std::vector<KeyType> result(uniqueNames.begin(), uniqueNames.end());
            return result;
        }
    };

    // ����ע����ģ���֧ࣨ����ʽ�ӿڣ�
    template<typename EnumType, typename ClassType, typename KeyType, typename KeyHash, typename KeyEqual,
        typename KeyToString, typename StringType, typename ErrorCallback>
        class PropertyRegistrar
    {
    public:
        PropertyRegistrar(PropertyData<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& propertyData, const StringType& className)
            : m_propertyData(propertyData), m_className(className)
        {
        }

        // ע�����ԣ���Ա������- ��ʽ�ӿڣ���������
        template<typename PropertyType>
        PropertyRegistrar& RegisterProperty(
            EnumType enumType,
            const KeyType& name,
            PropertyType ClassType::* memberPtr,
            const StringType& description = StringType())
        {
            // ����ƫ���� - ʹ�ÿ�ָ�뼼��
            size_t offset = reinterpret_cast<size_t>(
                &(reinterpret_cast<ClassType*>(0)->*memberPtr));

            // ����getter����
            std::function<void* (PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*)> getter =
                [memberPtr](PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* obj) -> void*
            {
                ClassType* derived = static_cast<ClassType*>(obj);
                return &(derived->*memberPtr);
            };

            // ����setter����
            std::function<void(PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*, void*)> setter =
                [memberPtr](PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* obj, void* value)
            {
                ClassType* derived = static_cast<ClassType*>(obj);
                derived->*memberPtr = *static_cast<PropertyType*>(value);
            };

            // ��������Ԫ����
            PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> meta;
            meta.name = name;
            meta.enumType = enumType;
            // ʹ��������ת��ΪStringType
            auto typeName = uectti::type_name<PropertyType>();
            meta.typeName = StringType(typeName.begin(), typeName.end());
            meta.offset = offset;
            meta.className = m_className;
            meta.getter = getter;
            meta.setter = setter;
            meta.isCustomAccessor = false;
            meta.registrationOrder = m_propertyData.registrationCounter++;
            meta.description = description;

            // ��¼ע��˳��
            m_propertyData.orderedPropertyNames.push_back(name);

            // ע�ᵽ��������
            m_propertyData.ownPropertyMap[name] = meta;

            // �洢������Ϣ
            if (!description.empty())
            {
                m_propertyData.descriptionMap[m_className][name] = description;
            }

            return *this;
        }

        // ע�����ԣ��Զ���getter��setter��- ��ʽ�ӿڣ���������
        template<typename PropertyType>
        PropertyRegistrar& RegisterProperty(
            EnumType enumType,
            const KeyType& name,
            void (ClassType::* setterFunc)(PropertyType&),
            PropertyType& (ClassType::* getterFunc)(),
            const StringType& description = StringType())
        {
            // ����getter����
            std::function<void* (PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*)> getter =
                [getterFunc](PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* obj) -> void*
            {
                ClassType* derived = static_cast<ClassType*>(obj);
                PropertyType& ref = (derived->*getterFunc)();
                return &ref;
            };

            // ����setter����
            std::function<void(PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>*, void*)> setter =
                [setterFunc](PropertyObject<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>* obj, void* value)
            {
                ClassType* derived = static_cast<ClassType*>(obj);
                (derived->*setterFunc)(*static_cast<PropertyType*>(value));
            };

            // ��������Ԫ����
            PropertyMeta<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback> meta;
            meta.name = name;
            meta.enumType = enumType;
            auto typeName = uectti::type_name<PropertyType>();
            meta.typeName = StringType(typeName.begin(), typeName.end());
            meta.offset = 0; // �����Զ����������ƫ����������
            meta.className = m_className;
            meta.getter = getter;
            meta.setter = setter;
            meta.isCustomAccessor = true;
            meta.registrationOrder = m_propertyData.registrationCounter++;
            meta.description = description;

            // ��¼ע��˳��
            m_propertyData.orderedPropertyNames.push_back(name);

            // ע�ᵽ��������
            m_propertyData.ownPropertyMap[name] = meta;

            // �洢������Ϣ
            if (!description.empty())
            {
                m_propertyData.descriptionMap[m_className][name] = description;
            }

            return *this;
        }

        // ע��ѡ�����ԣ���Ա������- ��ʽ�ӿڣ���������
        template<typename PropertyType>
        PropertyRegistrar& RegisterOptionalProperty(
            EnumType enumType,
            const KeyType& name,
            PropertyType ClassType::* memberPtr,
            std::initializer_list<const char*> options,
            const StringType& description = StringType())
        {
            // ����ע����ͨ����
            RegisterProperty(enumType, name, memberPtr, description);

            // ת��ѡ���б�
            std::vector<StringType> optionVec;
            for (const auto& option : options)
            {
                optionVec.push_back(StringType(option));
            }

            // Ȼ����Ϊѡ������
            auto& meta = m_propertyData.ownPropertyMap[name];
            meta.isOptional = true;

            // �洢��ѡ��ӳ����
            m_propertyData.optionalPropertyMap[m_className][name] = optionVec;

            // ��֤ѡ��ӳ��ֵ��0��ʼ������
            if (!optionVec.empty())
            {
                // ����Ƿ����ظ���ѡ���ַ���
                std::unordered_set<StringType> optionSet;
                for (const auto& option : optionVec)
                {
                    if (!optionSet.insert(option).second)
                    {
                        // ʹ�ô���ص��������
                        StringType warningMsg = StringType("Warning: Duplicate option string '") + option +
                            "' in property '" + KeyToString()(name) +
                            "' of class '" + m_className + "'";
                        ErrorCallback()(warningMsg);
                    }
                }
            }

            return *this;
        }

        // ע��ѡ�����ԣ��Զ���getter��setter��- ��ʽ�ӿڣ���������
        template<typename PropertyType>
        PropertyRegistrar& RegisterOptionalProperty(
            EnumType enumType,
            const KeyType& name,
            void (ClassType::* setterFunc)(PropertyType&),
            PropertyType& (ClassType::* getterFunc)(),
            std::initializer_list<const char*> options,
            const StringType& description = StringType())
        {
            // ����ע���Զ�������
            RegisterProperty(enumType, name, setterFunc, getterFunc, description);

            // ת��ѡ���б�
            std::vector<StringType> optionVec;
            for (const auto& option : options)
            {
                optionVec.push_back(StringType(option));
            }

            // Ȼ����Ϊѡ������
            auto& meta = m_propertyData.ownPropertyMap[name];
            meta.isOptional = true;

            // �洢��ѡ��ӳ����
            m_propertyData.optionalPropertyMap[m_className][name] = optionVec;

            // ��֤ѡ��ӳ��ֵ��0��ʼ������
            if (!optionVec.empty())
            {
                std::unordered_set<StringType> optionSet;
                for (const auto& option : optionVec)
                {
                    if (!optionSet.insert(option).second)
                    {
                        // ʹ�ô���ص��������
                        StringType warningMsg = StringType("Warning: Duplicate option string '") + option +
                            "' in property '" + KeyToString()(name) +
                            "' of class '" + m_className + "'";
                        ErrorCallback()(warningMsg);
                    }
                }
            }

            return *this;
        }

        // ������ע�����Ե�����
        PropertyRegistrar& SetDescription(const KeyType& name, const StringType& description)
        {
            auto it = m_propertyData.ownPropertyMap.find(name);
            if (it != m_propertyData.ownPropertyMap.end())
            {
                it->second.description = description;
                m_propertyData.descriptionMap[m_className][name] = description;
            }
            return *this;
        }

    private:
        PropertyData<EnumType, KeyType, KeyHash, KeyEqual, KeyToString, StringType, ErrorCallback>& m_propertyData;
        StringType m_className;
    };

} // namespace ROP

// ==================== �µ���ʽע��궨�� ====================

// �����꣺��ʼ������ϵͳ
#define INIT_PROPERTY_SYSTEM(ClassName, ParentClassName) \
    { \
        auto& propertyData = GetPropertyDataStatic(); \
        if (propertyData.initialized) return true; \
        \
        const ROPStringType classnamestring = ROPStringType(#ClassName); \
        \
        /* ����ע�Ḹ������Ե�����ӳ��� */ \
        if constexpr (!std::is_same_v<ParentClassName, ROP::PropertyObject<ROPEnumClass, ROPKeyType, ROPKeyHash, ROPKeyEqual, ROPKeyToString, ROPStringType, ROPErrorCallback>>) { \
            ParentClassName::StaticInitializeProperties(); \
            ROP::PropertySystemUtils<ROPEnumClass, ROPKeyType, ROPKeyHash, ROPKeyEqual, ROPKeyToString, ROPStringType, ROPErrorCallback>::RegisterParentProperties<ParentClassName>( \
                propertyData, ROPStringType(#ParentClassName)); \
        } \
        \
        ROPStringType ParentClassNameString = ROPStringType(#ParentClassName);

// �����꣺�������ϵͳ��ʼ�����ϲ���İ汾��
#define FINALIZE_PROPERTY_SYSTEM() \
        /* �������������б� */ \
        ROP::PropertySystemUtils<ROPEnumClass, ROPKeyType, ROPKeyHash, ROPKeyEqual, ROPKeyToString, ROPStringType, ROPErrorCallback>::BuildAllParentsNameList<ROPParentClassType>( \
            propertyData, ParentClassNameString); \
        \
        /* �������������б�ӳ�� */ \
        ROP::PropertySystemUtils<ROPEnumClass, ROPKeyType, ROPKeyHash, ROPKeyEqual, ROPKeyToString, ROPStringType, ROPErrorCallback>::BuildParentPropertiesListMap(propertyData); \
        \
        /* ʹ�úϲ�������ʼ���������� */ \
        ROP::PropertySystemUtils<ROPEnumClass, ROPKeyType, ROPKeyHash, ROPKeyEqual, ROPKeyToString, ROPStringType, ROPErrorCallback>::InitializePropertyData(propertyData); \
        \
        /* �������������б����������࣬����ͬ���� */ \
        ROP::PropertySystemUtils<ROPEnumClass, ROPKeyType, ROPKeyHash, ROPKeyEqual, ROPKeyToString, ROPStringType, ROPErrorCallback>::BuildAllPropertiesList(propertyData); \
        \
        propertyData.initialized = true; \
        return true; \
    }

// ���꣺�������������б�Ҫ����
#define DECLARE_OBJECT_WITH_PARENT(ClassName, ParentClassName) \
public:\
    virtual ROPStringType GetClassName() const override { \
        return ROPStringType(#ClassName); \
    } \
    virtual const ROPPropertyDataType& GetPropertyData() const override { \
        EnsurePropertySystemInitialized(); \
        return GetPropertyDataStatic(); \
    } \
    /* ��ȡ�������ݽṹ�壨��̬�汾�� */ \
    static ROPPropertyDataType& GetPropertyDataStatic() { \
        static ROPPropertyDataType s_propertyData; \
        return s_propertyData; \
    } \
protected: \
    using ROPClassType = ClassName;\
    using ROPParentClassType = ParentClassName;\
    \
    /* ��̬��ʼ������ */ \
    static bool StaticInitializeProperties() { \
        static bool s_initialized = []() -> bool { \
            INIT_PROPERTY_SYSTEM(ClassName, ParentClassName) \
            \
            /* ����ע�������� */ \
            ROP::PropertyRegistrar<ROPEnumClass, ClassName, ROPKeyType, ROPKeyHash, ROPKeyEqual, ROPKeyToString, ROPStringType, ROPErrorCallback> \
                registrar(propertyData, classnamestring); 

// �򻯺꣺����û�и�������
#define DECLARE_OBJECT(ClassName) DECLARE_OBJECT_WITH_PARENT(ClassName, ROPObjectType)

// ������
#define END_DECLARE_OBJECT() \
            /* �������ϵͳ��ʼ�� */ \
            FINALIZE_PROPERTY_SYSTEM() \
        }(); \
        return s_initialized; \
    } \
    \
    /* ȷ������ϵͳ�ѳ�ʼ�� */ \
    void EnsurePropertySystemInitialized() const { \
        static_cast<const ROPClassType*>(this)->StaticInitializeProperties(); \
    } \
    \
private:

////ʹ��ʾ��
//#include <ROP/RunTimeObjectProperty.h>
//#include <iostream>
//
//// ��������ö��
//enum class DeviceProperty
//{
//    ID,
//    NAME,
//    STATUS,
//    TEMPERATURE,
//    PRESSURE,
//    OPTIONAL
//};
//
//// �豸����
//class Device : public ROP::PropertyObject<DeviceProperty>
//{
//    DECLARE_OBJECT(Device)
//    registrar
//        .RegisterProperty(DeviceProperty::ID, "deviceId", &Device::deviceId, "�豸ID")
//        .RegisterProperty(DeviceProperty::NAME, "deviceName", &Device::deviceName, "�豸����")
//        .RegisterOptionalProperty(
//            DeviceProperty::OPTIONAL, "status", &Device::status,
//            { "Offline", "Online", "Error", "Maintenance" },
//            "�豸״̬");
//    END_DECLARE_OBJECT()
//
//public:
//    Device() : deviceId(0), status(0) {}
//
//    int deviceId;
//    std::string deviceName;
//    int status;  // 0:Offline, 1:Online, 2:Error, 3:Maintenance
//};
//
//// �¶ȴ�������
//class TemperatureSensor : public Device
//{
//    DECLARE_OBJECT_WITH_PARENT(TemperatureSensor, Device)
//    registrar
//        .RegisterProperty(
//            DeviceProperty::TEMPERATURE, "currentTemp", &TemperatureSensor::currentTemp,
//            "��ǰ�¶� (��C)")
//        .RegisterProperty(
//            DeviceProperty::TEMPERATURE, "targetTemp", &TemperatureSensor::targetTemp,
//            "Ŀ���¶� (��C)")
//        .RegisterOptionalProperty(
//            DeviceProperty::OPTIONAL, "unit", &TemperatureSensor::unit,
//            { "Celsius", "Fahrenheit", "Kelvin" },
//            "�¶ȵ�λ");
//    END_DECLARE_OBJECT()
//
//public:
//    TemperatureSensor() : currentTemp(20.0f), targetTemp(22.0f), unit(0) {}
//
//    float currentTemp;
//    float targetTemp;
//    int unit;  // 0:Celsius, 1:Fahrenheit, 2:Kelvin
//};
//
//int main()
//{
//    // �����¶ȴ�����
//    TemperatureSensor sensor;
//    sensor.deviceId = 1001;
//    sensor.deviceName = "LabSensor_01";
//    sensor.status = 1;  // Online
//    sensor.currentTemp = 21.5f;
//    sensor.targetTemp = 22.0f;
//    sensor.unit = 0;    // Celsius
//
//    // 1. ��ʾ�豸��Ϣ
//    std::cout << "=== �豸��Ϣ ===" << std::endl;
//    auto idProp = sensor.GetProperty("deviceId");
//    auto nameProp = sensor.GetProperty("deviceName");
//    auto statusProp = sensor.GetPropertyAsOptional("status");
//
//    std::cout << "ID: " << idProp.GetValue<int>() << std::endl;
//    std::cout << "Name: " << nameProp.GetValue<std::string>() << std::endl;
//    std::cout << "Status: " << statusProp.GetOptionString() << std::endl;
//
//    // 2. ��ʾ�¶���Ϣ
//    std::cout << "\n=== �¶���Ϣ ===" << std::endl;
//    auto tempProp = sensor.GetProperty("currentTemp");
//    auto targetProp = sensor.GetProperty("targetTemp");
//    auto unitProp = sensor.GetPropertyAsOptional("unit");
//
//    std::cout << "Current: " << tempProp.GetValue<float>() << "��C" << std::endl;
//    std::cout << "Target: " << targetProp.GetValue<float>() << "��C" << std::endl;
//    std::cout << "Unit: " << unitProp.GetOptionString() << std::endl;
//
//    // 3. ��̬��������
//    std::cout << "\n=== �������� ===" << std::endl;
//    unitProp.SetOptionByString("Fahrenheit");
//    std::cout << "Unit changed to: " << unitProp.GetOptionString() << std::endl;
//
//    // 4. ������������
//    std::cout << "\n=== �������� ===" << std::endl;
//    sensor.EnsurePropertySystemInitialized();
//    auto allProps = sensor.GetAllPropertiesOrdered();
//
//    for (const auto& prop : allProps)
//    {
//        std::cout << prop.GetName() << " (" << prop.GetClassName() << "): ";
//
//        try
//        {
//            if (prop.GetType() == DeviceProperty::ID)
//            {
//                std::cout << prop.GetValue<int>();
//            }
//            else if (prop.GetType() == DeviceProperty::NAME)
//            {
//                std::cout << "'" << prop.GetValue<std::string>() << "'";
//            }
//            else if (prop.GetType() == DeviceProperty::TEMPERATURE)
//            {
//                std::cout << prop.GetValue<float>();
//            }
//            else if (prop.GetType() == DeviceProperty::OPTIONAL)
//            {
//                auto optionalProp = sensor.ToOptionalProperty(prop);
//                std::cout << optionalProp.GetOptionString();
//            }
//        }
//        catch (...)
//        {
//            std::cout << "[Error reading value]";
//        }
//
//        std::cout << std::endl;
//    }
//
//    return 0;
//}
