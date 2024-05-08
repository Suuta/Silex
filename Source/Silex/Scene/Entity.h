
#pragma once

#include "Scene/Scene.h"
#include "Scene/Components.h"

#include <entt/entt.hpp>


namespace Silex
{
    class Entity
    {
    public:

        Entity() {};

        Entity(entt::entity handle, Scene* scene)
            : m_EntityHandle(handle)
            , m_Scene(scene)
        {
        }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            SL_ASSERT(!HasComponent<T>());
            T& component = m_Scene->registry.emplace<T>(m_EntityHandle, Traits::Forward<Args>(args)...);
            return component;
        }

        template<typename T>
        void RemoveComponent()
        {
            SL_ASSERT(HasComponent<T>());
            m_Scene->registry.remove<T>(m_EntityHandle);
        }

        template<typename T>
        T& GetComponent()
        {
            SL_ASSERT(HasComponent<T>());
            return m_Scene->registry.get<T>(m_EntityHandle);
        }

        template<typename T>
        bool HasComponent()
        {
            return m_Scene->registry.has<T>(m_EntityHandle);
        }

        operator bool()         const { return m_EntityHandle != entt::null; }
        operator entt::entity() const { return m_EntityHandle;               }
        operator uint32()       const { return (uint32)m_EntityHandle;       }

        uint64             GetID()   { return GetComponent<InstanceComponent>().id;   }
        const std::string& GetName() { return GetComponent<InstanceComponent>().name; }

        bool operator==(const Entity& other) const
        {
            return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
        }

        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

    private:

        entt::entity m_EntityHandle = entt::null;
        Scene*       m_Scene        = nullptr;

    private:

        friend class ScenePropertyPanel;
        friend class Scene;
    };
}