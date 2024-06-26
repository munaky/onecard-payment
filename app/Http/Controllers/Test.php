<?php

namespace App\Http\Controllers;

use Illuminate\Http\Request;
use App\Models\Murid;
use App\Models\Kelas;
use App\Models\Jurusan;
use App\Models\MuridSettings;
use App\Models\Users;
use App\Models\Role;
use App\Models\History;

class Test extends Controller
{
    public function __invoke(Request $req){
        $input = $req->all();
        $data = $this->models['murid']::with('murid_settings')->where('card_id', $input['card_id'])->first();
        return response()->json($data);
    }
}
