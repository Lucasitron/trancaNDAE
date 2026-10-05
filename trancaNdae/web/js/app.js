// Bootstrap: composição MVC (comandos voláteis da tranca).
import { ComandoView } from "./views/ComandoView.js";
import { ComandoController } from "./controllers/ComandoController.js";

new ComandoController(new ComandoView());
