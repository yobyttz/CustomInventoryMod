#include <string>
#include <vector>
#include <unordered_map>

struct ScriptCustomTab {
    std::string id;
    std::string title;
    std::string iconTexture;
    std::vector<std::string> itemIdentifiers;
};

class ScriptTabBridge {
private:
    std::unordered_map<std::string, ScriptCustomTab> scriptRegisteredTabs;

public:
    // Esta función es llamada cuando el Script API de JavaScript registra una pestaña
    void addTabFromScript(const std::string& id, const std::string& title, const std::string& icon, const std::vector<std::string>& items) {
        scriptRegisteredTabs[id] = {id, title, icon, items};
        
        // Aquí puedes actualizar automáticamente tu gestor visual de pestañas que usa el JSON UI
        // g_TabManager.registerTab(id, icon, title);
    }

    // Obtener los ítems que pertenecen a una pestaña específica al abrirla
    std::vector<std::string> getItemsForTab(const std::string& id) {
        if (scriptRegisteredTabs.find(id) != scriptRegisteredTabs.end()) {
            return scriptRegisteredTabs[id].itemIdentifiers;
        }
        return {};
    }
};

// Instancia global del puente de scripts
ScriptTabBridge g_ScriptTabBridge;
