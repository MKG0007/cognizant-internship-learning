# IT Service Request Portal (ITSRP) — Complete Project Documentation

---

# 1. Project Overview

## 1.1 Purpose and objective

The application is an **internal IT service desk portal** for an organisation. It replaces the informal "email IT and hope someone picks it up" workflow with a tracked, auditable system where:

- An **employee** raises an IT issue (broken laptop, missing access, software install), watches its status, deletes it if raised by mistake, and pushes back by re-opening it if IT closed it prematurely.
- An **IT administrator** sees every request in the organisation, searches for a specific person's requests, and closes them once resolved.

The core objective is that **no request can silently disappear** — every request has an ID, a raiser, a timestamp, and a status that only moves through defined transitions.

## 1.2 Architecture

Three physically separate projects in one Visual Studio solution, each owning exactly one concern:

```
┌──────────────────────────────────────────────────────────┐
│  PRESENTATION      Capstone.React (React 19 + Vite)      │
│  Browser UI, routing, role-based screen access            │
└───────────────────────────┬──────────────────────────────┘
                            │  HTTPS / JSON  (Axios)
                            ▼
┌──────────────────────────────────────────────────────────┐
│  SERVICE           Capstone.API (ASP.NET Core 8 Web API) │
│  HTTP endpoints, model validation, status codes, CORS    │
└───────────────────────────┬──────────────────────────────┘
                            │  C# method calls (DI)
                            ▼
┌──────────────────────────────────────────────────────────┐
│  DATA ACCESS       Capstone.DAL (.NET 8 class library)   │
│  Entities, EF Core DbContext, Repository                 │
└───────────────────────────┬──────────────────────────────┘
                            │  SQL (EF Core provider)
                            ▼
                    ServiceDeskDB.db (SQLite)
```

**The dependency direction is one-way and strict:**

- `Capstone.React` knows only the API's URL. It has no idea EF Core or SQLite exist.
- `Capstone.API` references `Capstone.DAL`. It knows about `IRepository` and the entity classes, but never writes a single line of SQL or touches `DbContext` directly.
- `Capstone.DAL` references nothing from the other two. It is the innermost layer and could be reused by a console app or a different frontend without modification.

This is the classic **layered (n-tier) architecture**, and it is what makes the project testable and swappable — replacing SQLite with SQL Server means changing one line in `Program.cs`, and replacing React with Angular means the API doesn't change at all.

## 1.3 Workflow — the request lifecycle

Everything in the application revolves around one state machine:

```
                          ┌──────────────────────┐
   User raises request    │                      │
   ──────────────────────▶│      1 = New         │
                          │                      │
                          └──────┬───────────────┘
                                 │ admin: Close Request
                                 ▼
                          ┌──────────────────────┐
                          │                      │
                          │     2 = Closed       │
                          │                      │
                          └──────┬───────────────┘
                                 │ user: Re-Open (justification required)
                                 ▼
                          ┌──────────────────────┐
                          │                      │
                          │    3 = Re-Opened     │◀──┐
                          │                      │   │
                          └──────┬───────────────┘   │
                                 │ admin: Close      │
                                 └───────────────────┘
```

Action availability is derived from this status, not stored separately:

| Status | Employee sees | Admin sees |
|---|---|---|
| New (1) | `Delete` | `Close Request` |
| Closed (2) | `Re-Open` | — |
| Re-Opened (3) | *(nothing)* | `Close Request` |

## 1.4 Major functionalities

1. **Authentication** — credential validation against the `Users` table; only seeded users can log in (no self-registration by design).
2. **Role-based authorisation** — `RoleId 1 = Admin`, `RoleId 2 = User`; drives which dashboard loads and which routes are reachable.
3. **Raise request** — employee submits Description + Details; server stamps `RaisedOn`, `RaisedBy`, and `ReqStatus = New`, returns the generated ID.
4. **View requests** — employee sees only their own; admin sees all.
5. **Delete request** — employee removes a New request permanently.
6. **Close request** — admin moves a request to Closed.
7. **Re-open request** — employee moves a Closed request to Re-Opened with a mandatory justification.
8. **Search requests** — admin filters all requests by requester name.

---

# 2. Technologies Used

## 2.1 Backend

### .NET 8 / C#
**Where:** `Capstone.API`, `Capstone.DAL`
**Why:** The target framework specified for the project. .NET 8 is an LTS release with built-in dependency injection, configuration, and logging — no third-party container or config library needed.
**How:** `Capstone.API` uses the `Microsoft.NET.Sdk.Web` SDK (gives it Kestrel, MVC, routing). `Capstone.DAL` uses the plain `Microsoft.NET.Sdk` SDK because it is a library with no web concerns.

### ASP.NET Core 8 Web API
**Where:** `Capstone.API/Program.cs`, `Controllers/ITSRPAPIController.cs`
**Why:** Provides the HTTP layer — routing, model binding, model validation, content negotiation, and status codes — without hand-writing any of it.
**How:** `[ApiController]` on the controller enables automatic model-binding from query strings and JSON bodies, and automatic `400 Bad Request` responses when Data Annotations fail. `[Route("api/[controller]")]` derives the base path `api/ITSRPAPI` from the class name.

### Entity Framework Core 9 (SQLite provider)
**Where:** `Capstone.DAL/Repository/HelpDeskDbContext.cs`, `Repository.cs`
**Why:** An ORM removes the need to write and maintain raw SQL for every CRUD operation, and its change tracker turns "load an object, change a property, call SaveChanges" into an `UPDATE` automatically.
**How:** `HelpDeskDbContext` exposes four `DbSet<T>` properties. LINQ queries against those `DbSet`s are translated to SQL by the SQLite provider at runtime. `Migrations/` holds the generated schema; `dotnet ef database update` applies it.

> EF Core **9.0.9** is used on a `net8.0` target — EF Core 9 officially supports .NET 8, so this combination is valid.

### SQLite
**Where:** `Capstone.DAL/Database/ServiceDeskDB.db`
**Why:** Specified by the requirements. It is a single-file, zero-configuration database — no server to install, and the `.db` file can be committed so any reviewer gets the schema and seed data instantly.
**How:** `Program.cs` computes the absolute path to the `.db` file at startup and passes it as the connection string. The path is built relative to the API's content root so it resolves identically on any machine.

### Swashbuckle (Swagger / OpenAPI) 6.6.2
**Where:** `Capstone.API/Program.cs`, served at `/swagger`
**Why:** Gives an interactive, browser-based test client for every endpoint without writing Postman collections — critical for testing the API in isolation before the React app existed.
**How:** `AddEndpointsApiExplorer()` + `AddSwaggerGen()` register the generator; `UseSwagger()` + `UseSwaggerUI()` serve it, gated behind `IsDevelopment()` so it isn't exposed in production.

### CORS (built into ASP.NET Core)
**Where:** `Capstone.API/Program.cs`
**Why:** The React dev server runs on `http://localhost:5173` while the API runs on `https://localhost:5193`. Different port = different origin, so the browser blocks the request unless the API explicitly permits it.
**How:** A named policy `_allowedOrigins` with `AllowAnyOrigin().AllowAnyMethod().AllowAnyHeader()`, applied via `app.UseCors(allowedOrigins)` — placed **before** `app.MapControllers()` so the middleware runs early enough to answer preflight `OPTIONS` requests.

### ILogger (built-in)
**Where:** Injected into `ITSRPAPIController`
**Why:** Turns swallowed exceptions into visible diagnostics. Without it, a caught exception returns a generic 500 to the client and vanishes.
**How:** `ILogger<ITSRPAPIController>` is resolved automatically by DI; every `catch` block calls `_logger.LogError(ex, "…", args)` with structured parameters before returning the error response.

## 2.2 Frontend

### React 19
**Where:** all of `Capstone.React/src`
**Why:** Component-based UI with hooks; the specified frontend framework.
**How:** Function components only. `useState` for form and list state, `useEffect` for data fetching on mount, `useCallback` to keep fetch functions stable across renders, `useContext` for the auth session, `useMemo` in the auth provider to avoid re-rendering every consumer on unrelated state changes.

### Vite 7
**Where:** build tooling for `Capstone.React`
**Why:** Near-instant dev server startup and HMR compared to webpack-based tooling.
**How:** `npm run dev` starts the dev server on port 5173; `npm run build` outputs a static bundle to `dist/`. Vite injects `.env` variables prefixed with `VITE_` into `import.meta.env` at build time.

### React Router (react-router-dom)
**Where:** `src/App.jsx`, `src/components/ProtectedRoute.jsx`
**Why:** The application has seven distinct screens; client-side routing gives each one a URL without a full page reload.
**How:** `<BrowserRouter>` wraps `<Routes>`. `ProtectedRoute` is used as a **layout route** — a `<Route element={<ProtectedRoute role="user" />}>` wrapping child routes, so the guard runs once for a whole group rather than being repeated on every page.

### Axios
**Where:** `src/api/apiClient.js`, `src/api/serviceRequestService.js`
**Why:** Chosen over `fetch` for three concrete reasons used in this project: a configurable base URL so the API host lives in one place, automatic JSON parsing, and **interceptors** — which is how the app would attach auth headers or handle global errors in one place rather than in every call.
**How:** A single `axios.create({ baseURL })` instance is exported and reused by every service function.

### Plain CSS (no framework)
**Where:** `src/index.css`
**Why:** Deliberately no Bootstrap/Tailwind. The visual identity (sky-blue and gold, serif headings, status pills) is custom, so a framework would have been overridden anyway and would only add bundle weight.
**How:** CSS custom properties on `:root` define the palette once; every rule references `var(--sky)`, `var(--gold)` etc. Media queries handle responsiveness, including converting tables into stacked cards on mobile.

## 2.3 Design patterns

| Pattern | Where | What it does here |
|---|---|---|
| **Repository** | `IRepository` / `Repository` | Hides EF Core behind a plain C# interface. The controller calls `_repository.ViewRequests("rahul")` and has no idea whether that is LINQ, raw SQL, or an in-memory list. |
| **Dependency Injection** | `Program.cs` → controller constructor | `AddScoped<IRepository, Repository>()` registers the mapping; ASP.NET Core constructs the controller and supplies the dependency. Swapping implementations means changing one registration line. |
| **Layered architecture** | The three projects | Enforced physically by project references, not just by folder naming. |
| **Context / Provider** | `AuthContext.jsx` | React's built-in DI. The session is held once at the tree root and read by any descendant via `useAuth()` — no prop drilling through seven page components. |
| **Service layer (frontend)** | `serviceRequestService.js` | Every HTTP call is a named function. Pages call `getAllRequests()`, never `axios.get(...)` — so an endpoint rename touches one file. |
| **Guard / Route protection** | `ProtectedRoute.jsx` | Centralises the "who may see this screen" decision instead of scattering `if (!user) navigate('/')` across pages. |
| **DTO-free entity reuse** | Entities cross all layers | A deliberate simplification for a project this size — `ServiceRequest` is used as both the EF entity and the API contract. Noted below as a known trade-off. |

---

# 3. Project Structure

```
CapstoneProject/
│
├── CapstoneSolution.sln              Visual Studio solution — ties the 3 projects together
├── azure-pipelines.yml               Azure DevOps CI/CD definition
├── .gitignore                        Excludes bin/, obj/, .vs/, node_modules/, dist/
├── README.md                         Entry point for reviewers
├── EVALUATOR_GUIDE.md                Credentials + feature walkthrough
│
├── Capstone.DAL/                     ── DATA ACCESS LAYER ──
│   ├── Capstone.DAL.csproj           net8.0, Nullable disabled, EF Core packages
│   ├── Model/                        Entity classes — the shape of the database
│   │   ├── Role.cs
│   │   ├── User.cs
│   │   ├── Status.cs
│   │   └── ServiceRequest.cs
│   ├── Repository/                   Data access contract + implementation
│   │   ├── HelpDeskDbContext.cs      EF Core context + design-time factory
│   │   ├── IRepository.cs            The interface the API depends on
│   │   └── Repository.cs             EF Core implementation
│   ├── Migrations/                   EF-generated schema history
│   │   ├── <timestamp>_InitialCreate.cs
│   │   ├── <timestamp>_InitialCreate.Designer.cs
│   │   └── HelpDeskDbContextModelSnapshot.cs
│   └── Database/
│       └── ServiceDeskDB.db          The actual SQLite database file
│
├── Capstone.API/                     ── SERVICE LAYER ──
│   ├── Capstone.API.csproj           net8.0 web SDK + ProjectReference to DAL
│   ├── Program.cs                    Composition root — DI, middleware, startup
│   ├── appsettings.json              Logging config
│   ├── Properties/
│   │   └── launchSettings.json       Dev profile — https://localhost:5193
│   └── Controllers/
│       └── ITSRPAPIController.cs     All 9 HTTP endpoints
│
└── Capstone.React/                   ── PRESENTATION LAYER ──
    ├── package.json                  Dependencies + npm scripts
    ├── vite.config.js                Vite/React plugin config
    ├── index.html                    HTML shell + Google Fonts links
    ├── .env                          VITE_API_BASE_URL
    ├── .gitignore                    Vite's own ignore rules (node_modules, dist)
    └── src/
        ├── main.jsx                  React entry point — mounts <App/>
        ├── App.jsx                   Route table + provider composition
        ├── index.css                 Entire design system
        ├── api/
        │   ├── apiClient.js          Configured Axios instance
        │   └── serviceRequestService.js   One function per endpoint
        ├── context/
        │   └── AuthContext.jsx       Session state + login/logout
        ├── components/
        │   ├── Layout.jsx            Banner + content + footer shell
        │   ├── ProtectedRoute.jsx    Route guard
        │   └── RequestTable.jsx      Shared request grid (3 screens use it)
        └── pages/
            ├── Login.jsx
            ├── UserHome.jsx
            ├── AddRequest.jsx
            ├── DeleteRequest.jsx
            ├── ReOpenRequest.jsx
            ├── AdminHome.jsx
            └── SearchRequests.jsx
```

## Folder responsibilities

| Folder | Responsibility | Rule it follows |
|---|---|---|
| `DAL/Model` | Declares *what* the data looks like and *what makes it valid* | Contains no logic — only properties and Data Annotations |
| `DAL/Repository` | All database access in the entire application | The only place `HelpDeskDbContext` is touched |
| `DAL/Migrations` | Schema version history | Auto-generated; never hand-edited |
| `DAL/Database` | The physical `.db` file | Path is resolved at runtime by `Program.cs` |
| `API/Controllers` | Translating HTTP ↔ repository calls | Contains no SQL, no `DbContext`, no business rules beyond validation and authorisation checks |
| `React/api` | All network communication | Pages never call Axios directly |
| `React/context` | Cross-cutting client state (the session) | Single source of truth for "who is logged in" |
| `React/components` | Reusable UI with no route of its own | `RequestTable` serves 3 screens; `Layout` serves all 7 |
| `React/pages` | One file per route | Each owns its own data fetching and form state |

---

# 4. File-by-File Explanation

## 4.1 DATA ACCESS LAYER — `Capstone.DAL`

---

### `Capstone.DAL.csproj`

**Purpose:** Defines the DAL as a .NET 8 class library and declares its two EF Core dependencies.

```xml
<PropertyGroup>
  <TargetFramework>net8.0</TargetFramework>
  <ImplicitUsings>enable</ImplicitUsings>
  <Nullable>disable</Nullable>
</PropertyGroup>
```

**Why `Nullable` is disabled here specifically:** `IRepository` declares methods like `User GetUser(string userName)` and `ServiceRequest GetRequestById(int requestId)`. Both legitimately return `null` when nothing is found — that is how the API detects "not found". With nullable reference types enabled, the compiler emits CS8603 warnings on every such return. Disabling it in this project only (the API keeps `Nullable` enabled) lets the interface signatures stay exactly as specified without warning noise.

**Packages:** `Microsoft.EntityFrameworkCore.Sqlite` (the provider that translates LINQ to SQLite SQL) and `Microsoft.EntityFrameworkCore.Tools` (supplies the `Add-Migration` / `Update-Database` PowerShell cmdlets; marked `PrivateAssets="all"` so it doesn't flow to projects that reference the DAL).

---

### `Model/Role.cs`

**Purpose:** Master table defining privilege levels.

```csharp
[Table("Roles")]
public class Role
{
    [Key]
    [DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    public int RoleId { get; set; }

    [Required(ErrorMessage = "Role name is required.")]
    [StringLength(35, ErrorMessage = "Role name cannot exceed 35 characters.")]
    public string RoleName { get; set; }
}
```

**How it works:** `[Key]` marks the primary key; `[DatabaseGenerated(Identity)]` makes SQLite auto-increment it. `[Table("Roles")]` pins the physical table name so it doesn't depend on EF's pluralisation.

**Data:** Exactly two rows, seeded once — `1 = Admin`, `2 = User`. These IDs are load-bearing: `AuthContext.jsx` compares `user.roleId === 1` to decide admin access.

**Interactions:** Referenced by `User.RoleId` as a foreign key, and loaded via `.Include(u => u.Role)` in `Repository.GetUser()`.

---

### `Model/User.cs`

**Purpose:** An application user. There is no registration screen — users exist only if seeded.

```csharp
[Table("Users")]
public class User
{
    [Key]
    [Required]
    [StringLength(20)]
    public string UserName { get; set; }

    [Required]
    [StringLength(20, MinimumLength = 8,
        ErrorMessage = "Password must be between 8 and 20 characters.")]
    public string Password { get; set; }

    [Required]
    [DataType(DataType.DateTime)]
    public DateTime CreatedOn { get; set; } = DateTime.Now;

    [Required]
    public int RoleId { get; set; }

    [ForeignKey(nameof(RoleId))]
    public Role Role { get; set; }
}
```

**Key design decision:** `UserName` is the **primary key**, not a surrogate int. This is why `ServiceRequest.RaisedBy` can store a plain string and still reliably identify a person — there is no ID indirection.

**The `Role` navigation property:** `[ForeignKey(nameof(RoleId))]` links it to the scalar FK. Because `Nullable` is disabled in this project, EF treats a reference navigation as **optional**, which is what we want — a `User` can be constructed for a login check without loading its `Role`.

**How it flows:** `Authenticate` builds a throwaway `User { UserName, Password }` purely as a parameter object. `GetUser` returns a fully-populated `User` including `Role`, which the API serialises to JSON and the React `AuthContext` stores in `sessionStorage`.

---

### `Model/Status.cs`

**Purpose:** Master table for the three request states.

```csharp
[Table("Status")]   // singular — matches the specified schema, not EF's default "Statuses"
public class Status
{
    [Key]
    [DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    public int StatusId { get; set; }

    [Required]
    [StringLength(50)]
    public string Description { get; set; }
}
```

**Note the mismatch that `[Table]` resolves:** `HelpDeskDbContext` declares `public DbSet<Status> Statuses { get; set; }`, which would normally produce a table named `Statuses`. The `[Table("Status")]` attribute overrides that so the physical table matches the specified schema.

**Data:** `1 = New`, `2 = Closed`, `3 = Re-Opened`. These IDs are hardcoded as constants in `Repository.cs` and mirrored in the React `STATUS_LABEL` map — the two must stay in sync.

---

### `Model/ServiceRequest.cs`

**Purpose:** The central entity — one IT request.

```csharp
[Table("ServiceRequests")]
public class ServiceRequest
{
    [Key]
    [DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    public int RequestId { get; set; }

    [Required] [StringLength(50)]  public string Description { get; set; }
    [Required] [StringLength(100)] public string Details { get; set; }
    [Required] [StringLength(20)]  public string RaisedBy { get; set; }

    [Required]
    [DataType(DataType.DateTime)]
    public DateTime RaisedOn { get; set; } = DateTime.Now;

    [Required] [StringLength(50)] public string Justification { get; set; }

    [Required]
    public int ReqStatus { get; set; }

    [ForeignKey(nameof(ReqStatus))]
    public Status Status { get; set; }
}
```

**The `Justification` design tension and how it is handled:** `Justification` is `[Required]` at the entity level, but the Add Request screen has no Justification field — it only becomes meaningful when re-opening. Two mechanisms cover this:

1. `AddRequest.jsx` sends `justification: "New Request"` as a placeholder so model binding passes validation.
2. `Repository.RaiseRequest()` defensively sets it to `"New Request"` if it arrives blank.

**The `ReqStatus` / `Status` pair:** `ReqStatus` is the integer FK actually stored in the table; `Status` is the navigation property populated by `.Include(r => r.Status)`. The React grid prefers `request.status?.description` (the human label from the join) and falls back to a local `STATUS_LABEL[reqStatus]` lookup if the include was missed.

---

### `Repository/HelpDeskDbContext.cs`

**Purpose:** The EF Core session — the bridge between entity classes and SQLite tables.

```csharp
public class HelpDeskDbContext : DbContext
{
    public HelpDeskDbContext(DbContextOptions<HelpDeskDbContext> options)
        : base(options) { }

    public class HelpDeskDbContextFactory : IDesignTimeDbContextFactory<HelpDeskDbContext>
    {
        public HelpDeskDbContext CreateDbContext(string[] args)
        {
            var options = new DbContextOptionsBuilder<HelpDeskDbContext>()
                .UseSqlite("Data Source=Database\\ServiceDeskDB.db")
                .Options;
            return new HelpDeskDbContext(options);
        }
    }

    public DbSet<Role> Roles { get; set; }
    public DbSet<User> Users { get; set; }
    public DbSet<ServiceRequest> ServiceRequests { get; set; }
    public DbSet<Status> Statuses { get; set; }
}
```

**Why there are two ways to construct it — this is the important part of this file.**

- **At runtime**, the context is created by dependency injection. `Program.cs` calls `AddDbContext<HelpDeskDbContext>(options => options.UseSqlite($"Data Source={dbPath}"))`, and DI supplies the `DbContextOptions` through the public constructor.
- **At design time** (`Add-Migration`, `Update-Database`), there is no running host and therefore no DI container. Without `IDesignTimeDbContextFactory`, the EF tooling would fail with "unable to create an object of type HelpDeskDbContext". The nested factory gives the tooling its own hardcoded connection string so migrations can be generated.

The four `DbSet<T>` properties are what LINQ queries hang off — `_context.ServiceRequests.Where(...)` becomes `SELECT * FROM ServiceRequests WHERE ...`.

---

### `Repository/IRepository.cs`

**Purpose:** The contract between the service layer and the data layer. This interface is the seam that keeps EF Core out of the API project.

```csharp
public interface IRepository
{
    bool Authenticate(User user);
    List<ServiceRequest> ViewRequests();
    List<ServiceRequest> ViewRequests(string userName);
    int RaiseRequest(ServiceRequest newRequest);
    ServiceRequest GetRequestById(int requestId);
    bool ReOpenRequest(ServiceRequest request);
    List<ServiceRequest> GetRequestBySP(string userName);
    bool CloseRequest(int requestId);
    bool DeleteRequest(int requestId);
    User GetUser(string userName);
}
```

**Design notes:**

- `ViewRequests()` is **overloaded** — no argument returns everything (admin home), a username returns one person's (user home and admin search). Same concept, two scopes.
- Return types are deliberately primitive-ish: `bool` for "did it work", `int` for the generated ID, entities or `List<T>` for data. The API layer converts these into HTTP status codes.
- `GetRequestBySP` exists because the specification called for a stored-procedure-style call. SQLite has no stored procedures, so it is implemented with a parameterised `FromSqlRaw` query — see below.

---

### `Repository/Repository.cs`

**Purpose:** The only class in the entire solution that talks to the database.

```csharp
public class Repository : IRepository
{
    private readonly HelpDeskDbContext _context;

    private const int StatusNew = 1;
    private const int StatusClosed = 2;
    private const int StatusReOpened = 3;

    public Repository(HelpDeskDbContext context) => _context = context;
```

The context arrives via constructor injection — the repository never news one up, so its lifetime is managed by ASP.NET Core (scoped: one per HTTP request).

**`Authenticate`** — the login check:
```csharp
return _context.Users.Any(u => u.UserName == user.UserName
                            && u.Password == user.Password);
```
`Any()` translates to `SELECT EXISTS(SELECT 1 FROM Users WHERE ...)` — it returns a boolean without materialising the row, which is exactly what is needed.

**`ViewRequests()` / `ViewRequests(userName)`** — the two grid queries:
```csharp
return _context.ServiceRequests
               .Include(r => r.Status)          // JOIN so the label comes back
               .Where(r => r.RaisedBy.ToLower() == userName.ToLower())
               .OrderBy(r => r.RequestId)
               .ToList();
```
`.Include()` is what makes `request.status.description` available to the React grid instead of just a bare integer. `.ToLower()` on both sides makes the match case-insensitive, so admin searching "Rahul" finds requests raised by "rahul".

**`RaiseRequest`** — the create path, and the one method that mutates incoming data:
```csharp
newRequest.RaisedOn = DateTime.Now;      // server owns the timestamp
newRequest.ReqStatus = StatusNew;        // server owns the initial status
if (string.IsNullOrWhiteSpace(newRequest.Justification))
    newRequest.Justification = "New Request";

_context.ServiceRequests.Add(newRequest);
_context.SaveChanges();
return newRequest.RequestId;             // populated by EF after the INSERT
```
The critical detail is the last line: `RequestId` is `0` before `SaveChanges()` and holds the database-generated identity value after it, because EF reads it back from SQLite. That value is what surfaces on screen as "Your request Id is 12".

**`ReOpenRequest`** — demonstrates EF's change tracker:
```csharp
var existing = _context.ServiceRequests
                       .FirstOrDefault(r => r.RequestId == request.RequestId);
if (existing == null) return false;

existing.Justification = request.Justification;
existing.ReqStatus = StatusReOpened;
_context.SaveChanges();
```
Note there is no `Update()` call. Because `existing` was loaded through the tracked context, EF compares its current property values against the originals on `SaveChanges()` and emits an `UPDATE` for only the two changed columns.

Note also that it **loads the entity fresh rather than trusting the posted object** — only `Justification` is taken from the request body. This prevents a client from overwriting `RaisedBy` or `RaisedOn` by posting a tampered payload.

**`GetRequestBySP`** — the raw-SQL variant:
```csharp
return _context.ServiceRequests
               .FromSqlRaw("SELECT * FROM ServiceRequests WHERE LOWER(RaisedBy) = LOWER({0})", userName)
               .Include(r => r.Status)
               .ToList();
```
The `{0}` placeholder is **parameterised, not interpolated** — EF converts it into a SQL parameter, so a username containing `'; DROP TABLE ...` is treated as a literal string, not executable SQL.

**`CloseRequest` / `DeleteRequest`** — both follow the same load-then-act shape and return `false` when the ID doesn't exist, which the controller translates into `404 Not Found`.

**Exception handling across every method:** each one wraps its body in `try/catch` and re-throws with a contextual message:
```csharp
catch (Exception ex)
{
    throw new Exception($"An error occurred while retrieving requests for '{userName}'.", ex);
}
```
The original exception is preserved as `InnerException`, so nothing is lost — but the caller gets a message that says which operation failed.

---

### `Migrations/`

Three generated files:
- **`<timestamp>_InitialCreate.cs`** — `Up()` contains the `CreateTable` calls for all four tables including FK constraints; `Down()` reverses them.
- **`<timestamp>_InitialCreate.Designer.cs`** — the model snapshot at the time this migration was created.
- **`HelpDeskDbContextModelSnapshot.cs`** — the *current* model state; EF diffs your entity classes against this file to work out what a new migration should contain.

These are never edited by hand. `dotnet ef database update` executes `Up()` against `ServiceDeskDB.db`.

---

## 4.2 SERVICE LAYER — `Capstone.API`

---

### `Capstone.API.csproj`

Uses `Microsoft.NET.Sdk.Web`, keeps `Nullable` **enabled** (unlike the DAL), references Swashbuckle, and — the line that wires the layers together:

```xml
<ProjectReference Include="..\Capstone.DAL\Capstone.DAL.csproj" />
```

This single reference is what gives the API access to `IRepository` and the entity classes. There is no reverse reference, which is what enforces the one-way dependency rule.

---

### `Program.cs`

**Purpose:** The composition root. Everything the application needs is registered and ordered here.

```csharp
var builder = WebApplication.CreateBuilder(args);

builder.Services.AddControllers()
    .AddJsonOptions(options =>
    {
        options.JsonSerializerOptions.ReferenceHandler = ReferenceHandler.IgnoreCycles;
    });
```
`IgnoreCycles` protects the serialiser: `ServiceRequest → Status` is currently a one-way navigation, but if a `Status → ICollection<ServiceRequest>` back-reference were ever added, serialisation would recurse infinitely without this.

```csharp
var dbPath = Path.Combine(
    Directory.GetParent(builder.Environment.ContentRootPath)!.FullName,
    "Capstone.DAL",
    "Database",
    "ServiceDeskDB.db");

builder.Services.AddDbContext<HelpDeskDbContext>(options =>
    options.UseSqlite($"Data Source={dbPath}"));
```
**Why the path is computed rather than hardcoded:** `ContentRootPath` is `.../CapstoneProject/Capstone.API`; its parent is `.../CapstoneProject`; from there the DAL's database folder is reachable. This resolves correctly on any developer's machine and on the build agent, without a machine-specific connection string in `appsettings.json`.

```csharp
builder.Services.AddScoped<IRepository, Repository>();
```
**The DI registration that makes the layering work.** `Scoped` means one `Repository` (and one `HelpDeskDbContext`) per HTTP request — the right lifetime for EF Core, since a context accumulates change-tracking state and must not be shared across concurrent requests.

```csharp
var allowedOrigins = "_allowedOrigins";
builder.Services.AddCors(options =>
{
    options.AddPolicy(name: allowedOrigins,
        policy => policy.AllowAnyOrigin().AllowAnyMethod().AllowAnyHeader());
});
```

**The middleware pipeline — order is behaviour, not style:**
```csharp
if (app.Environment.IsDevelopment()) { app.UseSwagger(); app.UseSwaggerUI(); }
app.UseHttpsRedirection();
app.UseCors(allowedOrigins);     // must precede MapControllers
app.UseAuthorization();
app.MapControllers();
app.Run();
```
`UseCors` sits before `MapControllers` because the browser's preflight `OPTIONS` request must be answered by the CORS middleware before routing ever tries to match it to an action. Swagger is inside the `IsDevelopment()` check so the API surface isn't published in production.

---

### `Properties/launchSettings.json`

Development-only file (never deployed). Its `https` profile fixes the port:

```json
"applicationUrl": "https://localhost:5193",
"launchUrl": "swagger"
```

That port is duplicated in exactly one other place — `Capstone.React/.env` — and the two must match or every frontend call fails with a network error.

---

### `Controllers/ITSRPAPIController.cs`

**Purpose:** Converts HTTP requests into repository calls and repository results into HTTP responses. It contains no data access and no business logic beyond validation.

```csharp
[Route("api/[controller]")]
[ApiController]
public class ITSRPAPIController : ControllerBase
{
    private readonly IRepository _repository;
    private readonly ILogger<ITSRPAPIController> _logger;

    public ITSRPAPIController(IRepository repository, ILogger<ITSRPAPIController> logger)
    {
        _repository = repository;
        _logger = logger;
    }
```

`[controller]` resolves to `ITSRPAPI` (the class name minus the `Controller` suffix), giving every endpoint the base path `/api/ITSRPAPI`. Both dependencies arrive by constructor injection — the controller depends on the **interface**, never on `Repository` directly.

**The nine endpoints:**

| Verb | Route | Method | Returns |
|---|---|---|---|
| GET | `/Authenticate?userName=&password=` | `Authenticate` | `200` + user, `404` if invalid |
| GET | `/GetAllRequest` | `GetAllRequest` | `200` + list, `404` if empty |
| GET | `/GetRequestByuserName?userName=` | `GetRequestByUN` | `200` + list (empty list is valid) |
| POST | `/CreateNewSerRequest` | `Post` | `201` + created request |
| GET | `/GetRequestById?reqId=` | `GetRequestById` | the entity, or `null` |
| POST | `/reopen` | `ReOpenRequest` | `200` + updated request |
| GET | `/CloseRequest?id=` | `CloseRequest` | `200` + message |
| GET | `/Delete?id=` | `Delete` | `200` + message |
| GET | `/GetUser?userName=` | `GetUser` | `200` + user, `404` if unknown |

**`Authenticate` — the entry point of every session:**
```csharp
var credentials = new User { UserName = userName, Password = password };

if (!_repository.Authenticate(credentials))
    return NotFound("Invalid user name or password.");

var user = _repository.GetUser(userName);
return Ok(user);
```
Two repository calls by design: the first proves the credentials, the second fetches the full record **including the `Role` navigation**, because the client needs `roleId` to decide which dashboard to load. Returning `404` rather than `401` for bad credentials is a deliberate contract choice — `serviceRequestService.authenticate()` catches exactly that status and resolves to `null`, which the login page renders as an inline error rather than an exception.

**`Post` (create) — the status-code decision:**
```csharp
if (!ModelState.IsValid) return BadRequest(ModelState);

var newId = _repository.RaiseRequest(newRequest);
if (newId <= 0) return BadRequest("The service request could not be created.");

return CreatedAtAction(nameof(GetRequestById), new { reqId = newId }, newRequest);
```
`ModelState.IsValid` is populated automatically by `[ApiController]` from the Data Annotations on `ServiceRequest` — if `Description` exceeds 50 characters, the request never reaches the repository. `CreatedAtAction` returns `201 Created` with a `Location` header pointing at the new resource, and the body carries the entity with its now-populated `RequestId` — which is what the React page reads to display "Your request Id is 12".

**`GetRequestById` — the one method that doesn't return `IActionResult`:**
```csharp
[HttpGet("GetRequestById")]
public ServiceRequest? GetRequestById(int reqId)
```
It returns the entity (or `null`) directly, which ASP.NET Core serialises to JSON or an empty response. The `?` was added to silence CS8603, since `null` is a legitimate return here. `DeleteRequest.jsx` and `ReOpenRequest.jsx` both check for a falsy result and show "Request id N was not found."

**Uniform exception handling:**
```csharp
catch (Exception ex)
{
    _logger.LogError(ex, "CloseRequest failed for {Id}", id);
    return StatusCode(StatusCodes.Status500InternalServerError,
        "An unexpected error occurred while closing the request.");
}
```
Two things happen: the full exception (including the repository's inner exception) is written to the server log with structured parameters, and the client receives a clean message with no stack trace or internal detail.

---

## 4.3 PRESENTATION LAYER — `Capstone.React`

---

### `.env`

```
VITE_API_BASE_URL=https://localhost:5193/api/ITSRPAPI
```

Vite exposes only `VITE_`-prefixed variables to client code, via `import.meta.env`. Because this is baked in at **build** time, the dev server must be restarted after editing it. Keeping the URL here rather than in code means dev/staging/prod can differ without a code change.

---

### `main.jsx`

The entry point. Mounts the app and imports the global stylesheet:
```jsx
createRoot(document.getElementById("root")).render(
  <StrictMode><App /></StrictMode>
);
```

---

### `App.jsx`

**Purpose:** Composes the providers and declares the entire route table. This file is the map of the application.

```jsx
<AuthProvider>
  <BrowserRouter>
    <Routes>
      <Route path="/" element={<Login />} />

      <Route element={<ProtectedRoute role="user" />}>
        <Route path="/home"        element={<UserHome />} />
        <Route path="/add"         element={<AddRequest />} />
        <Route path="/delete/:id"  element={<DeleteRequest />} />
        <Route path="/reopen/:id"  element={<ReOpenRequest />} />
      </Route>

      <Route element={<ProtectedRoute role="admin" />}>
        <Route path="/admin"  element={<AdminHome />} />
        <Route path="/search" element={<SearchRequests />} />
      </Route>

      <Route path="*" element={<Navigate to="/" replace />} />
    </Routes>
  </BrowserRouter>
</AuthProvider>
```

**Three structural decisions visible here:**
1. `AuthProvider` wraps `BrowserRouter`, so the session exists before any route renders and survives navigation.
2. `ProtectedRoute` is a **layout route** with no `path` of its own — it guards its children as a group. Adding a new admin screen means adding one line inside that block; the guard is inherited.
3. The `*` catch-all sends unknown URLs back to Login rather than showing a blank screen.

---

### `api/apiClient.js`

**Purpose:** One configured Axios instance for the whole application.

```js
const apiClient = axios.create({
  baseURL: import.meta.env.VITE_API_BASE_URL,
  headers: { "Content-Type": "application/json" },
});
```

Every service function calls `apiClient.get("/Authenticate")` rather than a full URL — so the host and base path exist in exactly one place.

---

### `api/serviceRequestService.js`

**Purpose:** One named function per API endpoint. This is the only file that knows endpoint names and parameter shapes.

```js
export const authenticate = async (userName, password) => {
  try {
    const { data } = await apiClient.get("/Authenticate", {
      params: { userName, password },
    });
    return data;
  } catch (error) {
    if (error.response && error.response.status === 404) return null;
    throw error;
  }
};
```

**The `404 → null` translation is the important pattern here.** "Wrong password" is a normal outcome, not a failure — so it is converted into a `null` return that `Login.jsx` renders as an inline message. Any *other* error (server down, 500, network failure) is re-thrown so the page can show "The service is unavailable" instead. `getAllRequests` uses the same idea, mapping `404` to an empty array so the admin grid renders "No service requests found" rather than crashing.

The file also exports the shared status constants:
```js
export const STATUS_LABEL = { 1: "New", 2: "Closed", 3: "Re-Opened" };
export const STATUS_NEW = 1;
export const STATUS_CLOSED = 2;
export const STATUS_REOPENED = 3;
```
These mirror the constants in `Repository.cs` — the one piece of knowledge deliberately duplicated across the stack.

---

### `context/AuthContext.jsx`

**Purpose:** Holds the session and exposes `login` / `logout` / `isAdmin` to every component.

```jsx
useEffect(() => {
  const saved = sessionStorage.getItem(STORAGE_KEY);
  if (saved) {
    try { setUser(JSON.parse(saved)); }
    catch { sessionStorage.removeItem(STORAGE_KEY); }
  }
  setLoading(false);
}, []);
```
**Why `loading` exists:** on a hard refresh of `/home`, React mounts before `sessionStorage` has been read. Without the `loading` flag, `ProtectedRoute` would see `user === null`, conclude "not authenticated", and bounce the user to Login on every refresh. `ProtectedRoute` returns `null` while `loading` is true, so the guard waits for the restore to finish.

```jsx
const login = async (userName, password) => {
  const found = await authenticate(userName, password);
  if (!found) return null;
  setUser(found);
  sessionStorage.setItem(STORAGE_KEY, JSON.stringify(found));
  return found;
};
```
Two writes: React state (drives re-render) and `sessionStorage` (survives refresh). `logout` clears both.

```jsx
const value = useMemo(() => ({
  user, loading, login, logout,
  isAuthenticated: Boolean(user),
  isAdmin: user?.roleId === ADMIN_ROLE_ID,   // ADMIN_ROLE_ID = 1
}), [user, loading]);
```
`useMemo` keeps the context value object stable between renders, so consumers don't re-render on every parent render. `isAdmin` is derived here once rather than being recomputed in each page.

`sessionStorage` (not `localStorage`) is deliberate — closing the tab ends the session.

---

### `components/ProtectedRoute.jsx`

**Purpose:** Enforces "anonymous users may only see Login", plus role separation.

```jsx
if (loading) return null;
if (!isAuthenticated)          return <Navigate to="/" replace />;
if (role === "admin" && !isAdmin) return <Navigate to="/home" replace />;
if (role === "user"  &&  isAdmin) return <Navigate to="/admin" replace />;
return <Outlet />;
```

`<Outlet />` renders whichever child route matched. `replace` prevents the blocked URL from entering browser history, so Back doesn't bounce the user into a redirect loop.

---

### `components/Layout.jsx`

**Purpose:** The page shell every screen renders inside — banner, content slot, footer.

Takes a `showLogout` prop, set to `false` only by `Login.jsx` (nobody is logged in there). Reads `user` from context to display the username beside the Logout button, and calls `logout()` followed by `navigate("/", { replace: true })`.

---

### `components/RequestTable.jsx`

**Purpose:** The single grid component used by `UserHome`, `AdminHome`, and `SearchRequests`. It is the most reused component in the project.

**Props:**
| Prop | Effect |
|---|---|
| `requests` | The array to render |
| `mode` | `"user"` or `"admin"` — decides which action links appear |
| `showDetails` | Adds the Details column (used only by admin search) |
| `onCloseRequest` | Admin-only callback |

**Action logic — the state machine expressed in JSX:**
```jsx
const isNew      = request.reqStatus === STATUS_NEW;
const isClosed   = request.reqStatus === STATUS_CLOSED;
const isReopened = request.reqStatus === 3;

{mode === "user"  && isNew      && <Link to={`/delete/${id}`}>Delete</Link>}
{mode === "user"  && isClosed   && <Link to={`/reopen/${id}`}>Re-Open</Link>}
{mode === "admin" && !isClosed  && <button onClick={() => onCloseRequest(id)}>Close Request</button>}
```
`Delete` is gated on `isNew` specifically (not `!isClosed`), which is why a Re-Opened request shows no user action — a deliberate decision for a state the original specification never depicted.

**Status rendering with fallback:**
```jsx
const statusOf = (r) => r.status?.description ?? STATUS_LABEL[r.reqStatus] ?? "";
```
Prefers the joined label from the API; falls back to the local map if `.Include()` was missed.

**Justification display without a new column:**
```jsx
{hasJustification && (
  <div className="description-note">
    <span className="description-note-label">Justification:</span> {request.justification}
  </div>
)}
```
Only renders for Re-Opened requests with non-empty justification, nested inside the existing Description cell — so the admin can see why a request was reopened without altering the table's column structure.

`Delete`/`Re-Open` stay as `<Link>` elements (correct semantics for navigation — right-click, open-in-new-tab, screen readers) while `Close Request` is a real `<button>` (an in-place action, no navigation). Both are styled identically via `.row-action`.

---

### `pages/Login.jsx`

The only anonymous screen. Validates that both fields are non-empty, calls `login()`, and branches on the returned role:
```jsx
const user = await login(form.userName.trim(), form.password);
if (!user) { setError("Invalid EmailId or Password."); return; }
navigate(user.roleId === 1 ? "/admin" : "/home", { replace: true });
```
The `busy` flag disables the submit button during the request to prevent double submission. `replace: true` keeps Login out of history, so Back from a dashboard doesn't return to the login form.

---

### `pages/UserHome.jsx`

```jsx
const loadRequests = useCallback(async () => {
  try {
    const data = await getRequestsByUserName(user.userName);
    setRequests(data);
  } catch { setMessage("Could not load your service requests."); }
}, [user.userName]);

useEffect(() => { loadRequests(); }, [loadRequests]);
```
`useCallback` keeps `loadRequests` referentially stable so the `useEffect` dependency doesn't change on every render and cause an infinite fetch loop. Renders `<RequestTable mode="user" />`.

---

### `pages/AddRequest.jsx`

Owns form state and client-side validation that **mirrors the server's Data Annotations** (required + max length on both fields) — the server still validates independently; the client copy just gives instant feedback.

```jsx
const created = await createRequest({
  description: form.description.trim(),
  details: form.details.trim(),
  raisedBy: user.userName,
  justification: "New Request",
  reqStatus: STATUS_NEW,
});
setSuccess(`Request Added Successfully. Your request Id is ${created.requestId}`);
setForm({ description: "", details: "" });
```
`created.requestId` is the database-generated ID travelling all the way back from `SaveChanges()`. The form clears on success so a second request can be raised immediately.

Structurally, each label/input/error trio is wrapped in `.form-field` and the button in `.form-actions`, so spacing stays constant whether or not an error message is visible.

---

### `pages/DeleteRequest.jsx` and `pages/ReOpenRequest.jsx`

Both read the ID from the URL via `useParams()`, fetch the single record with `getRequestById()`, and render it read-only.

`DeleteRequest` sets a `deleted` flag after success to disable the button and prevent a second call.

`ReOpenRequest` is the only screen with a mandatory input:
```jsx
if (!justification.trim()) {
  setError("Justification is mandatory to re-open a request.");
  return;
}
await reOpenRequest({ ...request, justification: justification.trim() });
setTimeout(() => navigate("/home"), 1200);
```
It spreads the existing request and overrides only `justification` — and the server independently reloads the entity and takes only that field, so the client cannot tamper with `RaisedBy` or `RaisedOn`.

---

### `pages/AdminHome.jsx`

Fetches all requests and handles closing in place:
```jsx
const handleClose = async (requestId) => {
  await closeRequest(requestId);
  await loadRequests();     // refetch so the status column updates
};
```
Refetching rather than mutating local state guarantees the grid reflects what is actually in the database.

---

### `pages/SearchRequests.jsx`

Uses a three-state pattern for results:
```jsx
const [results, setResults] = useState(null);   // null = no search run yet
```
`null` renders nothing, `[]` renders "No matching requests found", and a populated array renders the grid with `showDetails`. Closing a request from the results refetches the same search rather than reloading everything.

---

### `index.css`

The entire design system in one file:
- **`:root` custom properties** — palette (`--sky`, `--gold`, status colours), spacing, radius. Changing the theme means editing this block only.
- **Component rules** — `.banner`, `.login-box`, `.stacked-form`, `.request-table`, `.status-pill`, `.row-action`, `.nav-link`.
- **Responsive rules** — below 860px content padding tightens; below 640px the request table's `<thead>` is hidden and each `<td>` becomes a labelled row using `content: attr(data-label)` from the `data-label` attributes set in `RequestTable.jsx`, turning each record into a stacked card. A `(hover: none) and (pointer: coarse)` query enforces 44px minimum touch targets on any touch device.

---

## 4.4 Root files

### `azure-pipelines.yml`

Stages: `ApplicationBuild → UnitTest → deploy_dev → functionaltest_dev → staging → prod`.

`ApplicationBuild` restores all `.csproj` files, publishes with `publishWebProjects: true` (which auto-selects `Capstone.API` because it is the only web SDK project — the DAL class library is correctly excluded), zips the output, and publishes it as the `drop` artifact. `deploy_dev` downloads `drop` and pushes it to the Azure Web App. Staging and prod are gated behind `deploy_staging` / `deploy_prod` variables and skip unless set to `'yes'`.

### `.gitignore`

Excludes `bin/`, `obj/`, `.vs/`, `node_modules/`, `dist/`. The `.db` file is deliberately **not** ignored so reviewers receive the schema and seed data with the clone.

---

# 5. Functionality Implementation

Each feature is traced from the click to the database and back.

## 5.1 Login

```
Login.jsx  handleSubmit()
  └─▶ AuthContext.login(userName, password)
        └─▶ serviceRequestService.authenticate()
              └─▶ apiClient.get("/Authenticate", { params })
                    ══ HTTP GET https://localhost:5193/api/ITSRPAPI/Authenticate?userName=rahul&password=Rahul@123
                    └─▶ ITSRPAPIController.Authenticate()
                          ├─▶ IRepository.Authenticate(new User{...})
                          │     └─▶ _context.Users.Any(u => u.UserName == ... && u.Password == ...)
                          │           ══ SELECT EXISTS(SELECT 1 FROM Users WHERE UserName = @p0 AND Password = @p1)
                          └─▶ IRepository.GetUser(userName)
                                └─▶ _context.Users.Include(u => u.Role).FirstOrDefault(...)
                                      ══ SELECT ... FROM Users u LEFT JOIN Roles r ON u.RoleId = r.RoleId WHERE ...
                    ◀── 200 OK { userName, password, createdOn, roleId, role: { roleId, roleName } }
              ◀── data
        ├── setUser(found)                          → React state
        └── sessionStorage.setItem("itsrp.user", …) → survives refresh
  └─▶ navigate(user.roleId === 1 ? "/admin" : "/home", { replace: true })
```

**Failure path:** repository returns `false` → controller returns `404` → `authenticate()` catches status 404 and returns `null` → `login()` returns `null` → `Login.jsx` shows "Invalid EmailId or Password." No exception is thrown; the failure is a normal branch.

**Dependencies:** `Login.jsx` → `AuthContext` → `serviceRequestService` → `apiClient`. The page never touches Axios and never sees the API URL.

## 5.2 Raise a new request

```
AddRequest.jsx  handleSubmit()
  ├── validate()                     ← client-side mirror of Data Annotations
  └─▶ createRequest({ description, details, raisedBy: user.userName,
                      justification: "New Request", reqStatus: 1 })
        └─▶ apiClient.post("/CreateNewSerRequest", body)
              ══ HTTP POST … Content-Type: application/json
              └─▶ [ApiController] binds JSON → ServiceRequest
                    └─▶ validates Data Annotations → ModelState
                          └─▶ ITSRPAPIController.Post()
                                └─▶ IRepository.RaiseRequest(newRequest)
                                      ├── RaisedOn  = DateTime.Now      ← server-owned
                                      ├── ReqStatus = 1 (New)           ← server-owned
                                      ├── _context.ServiceRequests.Add(entity)
                                      └── _context.SaveChanges()
                                            ══ INSERT INTO ServiceRequests (…) VALUES (…);
                                            ══ SELECT last_insert_rowid();
                                      ◀── newRequest.RequestId now populated
              ◀── 201 Created + body { requestId: 12, … }
  └── setSuccess(`Request Added Successfully. Your request Id is 12`)
  └── setForm({ description: "", details: "" })
```

**Where each field comes from:** the client supplies `description`, `details`, and `raisedBy`; the server overwrites `raisedOn` and `reqStatus` regardless of what was posted. Validation runs twice — once in `validate()` for immediate feedback, once in `ModelState` as the authoritative check.

## 5.3 View requests (both dashboards)

The same component and the same repository method, differing only in scope:

**Employee** — `UserHome` → `getRequestsByUserName(user.userName)` → `GetRequestByUN` → `ViewRequests(userName)` → `WHERE LOWER(RaisedBy) = LOWER(@p0)`.

**Admin** — `AdminHome` → `getAllRequests()` → `GetAllRequest` → `ViewRequests()` → no `WHERE` clause.

Both queries `.Include(r => r.Status)`, so each row arrives with a nested `status` object, and both render through `<RequestTable>` — the `mode` prop is the only thing that changes which action links appear.

## 5.4 Delete a request

```
UserHome  ─ Delete link ─▶ /delete/12
  DeleteRequest.jsx  useEffect
    └─▶ getRequestById(12) ─▶ GET /GetRequestById?reqId=12
          └─▶ Repository.GetRequestById()   .AsNoTracking()  ← read-only, no change tracking
    ◀── entity rendered read-only in a confirmation table

  User clicks "Delete Request"
    └─▶ deleteRequest(12) ─▶ GET /Delete?id=12
          └─▶ Repository.DeleteRequest(12)
                ├── FirstOrDefault(r => r.RequestId == 12)
                ├── null? → return false → controller returns 404
                └── _context.ServiceRequests.Remove(entity); SaveChanges()
                      ══ DELETE FROM ServiceRequests WHERE RequestId = 12
    ◀── 200 OK "Request id 12 has been deleted successfully."
  └── setDeleted(true)   ← disables the button, preventing a second call
```

## 5.5 Close a request (admin)

```
AdminHome  ─ Close Request button ─▶ handleClose(8)
  └─▶ closeRequest(8) ─▶ GET /CloseRequest?id=8
        └─▶ Repository.CloseRequest(8)
              ├── load entity (tracked)
              ├── request.ReqStatus = 2
              └── SaveChanges()  ══ UPDATE ServiceRequests SET ReqStatus = 2 WHERE RequestId = 8
  └─▶ await loadRequests()      ← refetch, not local mutation
        └── grid re-renders: pill becomes "Closed", Close button disappears
```

The refetch is what guarantees the UI reflects the database rather than an optimistic guess.

## 5.6 Re-open a request

```
UserHome  ─ Re-Open link (only on Closed rows) ─▶ /reopen/9
  ReOpenRequest.jsx  loads the request read-only, Justification is the only editable field

  User submits
    ├── client guard: justification non-empty and ≤ 50 chars
    └─▶ reOpenRequest({ ...request, justification }) ─▶ POST /reopen
          └─▶ ModelState validation
                └─▶ controller re-checks Justification is non-empty → 400 if blank
                      └─▶ Repository.ReOpenRequest(request)
                            ├── load the CURRENT entity from the database
                            ├── copy ONLY Justification from the payload
                            ├── ReqStatus = 3 (Re-Opened)
                            └── SaveChanges()  ══ UPDATE … SET Justification = @p0, ReqStatus = 3 …
    ◀── 200 OK + updated entity
  └── setTimeout(() => navigate("/home"), 1200)
```

**Three independent checks on Justification:** client-side guard, `[Required]` via `ModelState`, and an explicit controller check. Defence in depth — the client check is convenience, the server checks are the actual guarantee.

Afterwards the row shows a `Re-Opened` pill, no user action link, and — on the admin's screens — the justification text appears beneath the description.

## 5.7 Search requests (admin)

```
SearchRequests.jsx  handleSearch()
  └─▶ getRequestsByUserName("rahul")   ← same endpoint the user home uses
        └─▶ GetRequestByUN → ViewRequests("rahul")
  └── results === null  → render nothing (no search yet)
      results.length===0 → "No matching requests found."
      results.length > 0 → <RequestTable mode="admin" showDetails />
```

Closing from search results calls `runSearch(name)` again rather than `loadRequests()`, preserving the current filter.

## 5.8 Authorisation

Enforced in one place, consumed everywhere:

```
AuthProvider  →  isAuthenticated, isAdmin  →  ProtectedRoute  →  every guarded route
```

| Attempt | Result |
|---|---|
| Anonymous visits `/home` | `!isAuthenticated` → `<Navigate to="/" />` |
| Employee visits `/admin` | `role === "admin" && !isAdmin` → `<Navigate to="/home" />` |
| Admin visits `/home` | `role === "user" && isAdmin` → `<Navigate to="/admin" />` |
| Any user refreshes `/home` | `loading === true` → guard returns `null` and waits for the sessionStorage restore |
| Unknown URL | `*` route → `<Navigate to="/" />` |

## 5.9 Component dependency graph

```
main.jsx
  └── App.jsx
        ├── AuthProvider ──────────── serviceRequestService ── apiClient ── .env
        │        │                            │
        │        └── useAuth() consumed by:   └── all 8 service functions
        │              ProtectedRoute, Layout, Login, UserHome,
        │              AddRequest, AdminHome
        │
        └── BrowserRouter
              ├── Login ─────────────── Layout
              ├── ProtectedRoute(user)
              │     ├── UserHome ────── Layout + RequestTable + getRequestsByUserName
              │     ├── AddRequest ──── Layout + createRequest
              │     ├── DeleteRequest ─ Layout + getRequestById + deleteRequest
              │     └── ReOpenRequest ─ Layout + getRequestById + reOpenRequest
              └── ProtectedRoute(admin)
                    ├── AdminHome ───── Layout + RequestTable + getAllRequests + closeRequest
                    └── SearchRequests ─ Layout + RequestTable + getRequestsByUserName + closeRequest
```

`Layout` is used by all seven pages. `RequestTable` is used by three. `serviceRequestService` is the only module that imports `apiClient`.

---

# 6. Overall Execution Flow

## 6.1 What happens when the API starts

```
dotnet run --launch-profile https
  │
  1. launchSettings.json → ASPNETCORE_ENVIRONMENT = Development, URL = https://localhost:5193
  │
  2. WebApplication.CreateBuilder(args)
     └── loads appsettings.json + appsettings.Development.json + environment variables
     └── configures the default console logger
  │
  3. Service registration (nothing executes yet — only recorded in the DI container)
     ├── AddControllers() + JSON options
     ├── AddSwaggerGen()
     ├── dbPath computed  → …/CapstoneProject/Capstone.DAL/Database/ServiceDeskDB.db
     ├── AddDbContext<HelpDeskDbContext>(UseSqlite(dbPath))     [Scoped]
     ├── AddScoped<IRepository, Repository>()                    [Scoped]
     └── AddCors("_allowedOrigins")
  │
  4. builder.Build()  → the DI container is finalised
  │
  5. Middleware pipeline assembled, in order:
     Swagger → HttpsRedirection → Cors → Authorization → MapControllers
  │
  6. app.Run()  → Kestrel binds port 5193 and begins listening
```

No database connection is opened at startup — EF Core connects lazily on the first query.

## 6.2 What happens when the React app starts

```
npm run dev
  │
  1. Vite reads .env, injects VITE_API_BASE_URL into import.meta.env
  2. Dev server serves index.html on http://localhost:5173
  3. index.html loads Google Fonts and mounts /src/main.jsx
  4. main.jsx → createRoot(#root).render(<StrictMode><App /></StrictMode>)
  5. App.jsx mounts AuthProvider
       └── useEffect runs → reads sessionStorage["itsrp.user"]
             ├── found  → setUser(parsed), loading = false
             └── absent → loading = false, user stays null
  6. BrowserRouter matches the current URL
       └── "/" → <Login />
       └── any guarded path → ProtectedRoute evaluates (waits while loading === true)
```

## 6.3 A complete request-to-response cycle

Taking "admin closes request 8" end to end:

```
 1  Browser        Admin clicks "Close Request" on row 8
 2  React          handleClose(8) → closeRequest(8)
 3  Axios          GET https://localhost:5193/api/ITSRPAPI/CloseRequest?id=8
                   (browser first sends an OPTIONS preflight)
 4  Kestrel        receives the request
 5  Middleware     HttpsRedirection → CORS answers the preflight, then allows the GET
 6  Routing        matches [HttpGet("CloseRequest")] on ITSRPAPIController
 7  DI             creates a scope:
                     new HelpDeskDbContext(options)
                     new Repository(context)
                     new ITSRPAPIController(repository, logger)
 8  Binding        query string "id=8" → int id = 8
 9  Controller     _repository.CloseRequest(8)
10  Repository     _context.ServiceRequests.FirstOrDefault(r => r.RequestId == 8)
11  EF Core        SELECT … FROM ServiceRequests WHERE RequestId = 8 LIMIT 1
12  SQLite         reads ServiceDeskDB.db, returns the row
13  EF Core        materialises the entity and begins tracking it
14  Repository     request.ReqStatus = 2
15  Repository     _context.SaveChanges()
16  EF Core        change tracker detects one modified column
17  SQLite         UPDATE ServiceRequests SET ReqStatus = 2 WHERE RequestId = 8
18  Repository     returns true
19  Controller     return Ok("Request id 8 has been closed.")
20  Middleware     response serialised, CORS headers attached
21  DI             scope disposed → DbContext disposed → connection released
22  Axios          promise resolves
23  React          await loadRequests() → GET /GetAllRequest (steps 3–22 repeat)
24  React          setRequests(newData) → re-render
25  Browser        row 8 now shows a "Closed" pill; its Close button is gone
```

**The DI scope (steps 7 and 21) is the part worth internalising:** a fresh `DbContext` and `Repository` are created for every single HTTP request and destroyed when it completes. Nothing leaks between requests, and concurrent requests never share a change tracker.

## 6.4 How the state machine is enforced end to end

The lifecycle is protected at three levels, and no single level is trusted alone:

| Level | Mechanism |
|---|---|
| **UI** | `RequestTable` only renders the action links that are legal for the row's current status |
| **Repository** | `RaiseRequest` always sets status 1; `CloseRequest` always sets 2; `ReOpenRequest` always sets 3 — the client never chooses a status |
| **Database** | `ReqStatus` is a foreign key into `Status`, so only 1, 2, or 3 can ever be stored |

Even a hand-crafted HTTP request cannot put a record into an invalid state, because the transition value is decided server-side in every case.

## 6.5 Known trade-offs

Documented deliberately rather than left implicit:

1. **Passwords are stored and compared in plain text.** The specification's `Authenticate(User)` compares raw strings. Hashing with BCrypt would be the production fix.
2. **Authorisation is client-enforced.** `ProtectedRoute` decides access from `sessionStorage`, so the API endpoints themselves are open to a direct HTTP client. JWT with `[Authorize(Roles = "Admin")]` was prototyped and deliberately not adopted, to stay aligned with what the specification explicitly required.
3. **Entities double as API contracts.** `ServiceRequest` is both the EF entity and the JSON payload. Separate DTOs would decouple the wire format from the schema, at the cost of mapping code disproportionate to a project this size.
4. **SQLite on Azure App Service.** The file lives inside the deployed app, so writes are lost on redeploy or scale-out. Azure SQL would be the production choice.
